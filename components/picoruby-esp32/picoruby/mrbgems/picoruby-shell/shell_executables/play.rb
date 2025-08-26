require "filesystem-fat"
require "nes-apu"

#   1. ヘッダー構造
#   オフセット| サイズ | 内容
#   ---------|-------|-----
#   0x00     | 8     | magic: "APULOG\0\0"
#   0x08     | 4     | version: 2 (INIT/PLAY対応版)
#   0x0C     | 4     | entry_count: エントリー総数
#   0x10     | 4     | frame_count: 総フレーム数
#   0x14     | 12    | reserved[3]: 将来用（0で埋める）
#   ---------|-------|-----
#   合計     | 32    | ヘッダーサイズ

#   2. エントリー構造
#   オフセット| サイズ | 内容
#   ---------|-------|-----
#   0x00     | 4     | time: フレーム開始からの相対時間（CPUサイクル）
#   0x04     | 2     | addr: レジスタアドレス (0x4000-0x4017) または 0xFFFF（イベント）
#   0x06     | 1     | data: 書き込みデータ
#   0x07     | 1     | event_type: イベントタイプ（下記参照）
#   0x08     | 4     | frame_number: フレーム番号
#   ---------|-------|-----
#   合計     | 12    | エントリーサイズ

#   3. イベントタイプ (apu_log_event_type)
#   値  | 意味
#   ----|-----
#   0   | APU_EVENT_WRITE - 通常のAPU書き込み
#   1   | APU_EVENT_INIT_START - INIT開始
#   2   | APU_EVENT_INIT_END - INIT終了
#   3   | APU_EVENT_PLAY_START - PLAYルーチン開始
#   4   | APU_EVENT_PLAY_END - PLAYルーチン終了

class ApuRegLog
    # APU Event Types
    APU_EVENT_WRITE = 0
    APU_EVENT_INIT_START = 1
    APU_EVENT_INIT_END = 2
    APU_EVENT_PLAY_START = 3
    APU_EVENT_PLAY_END = 4

    ENTRY_SIZE = 12

    attr_reader :header, :current_frame
    
    def initialize(path)
        @path = path
        @current_frame = 0
        @current_pos = 0
        puts "loading... #{@path}"        

        File.open(@path, "r") do |f|
            # Read header (32 bytes)
            magic = f.read(8)
            puts "  magic: #{magic.inspect}"
            
            # Read version (4 bytes, little-endian)
            version_bytes = f.read(4)
            version = read_uint32_le(version_bytes)
            puts "  version: #{version}"
            
            # Read entry_count (4 bytes, little-endian)
            entry_count = read_uint32_le(f.read(4))
            puts "  entry_count: #{entry_count}"
            
            # Read frame_count (4 bytes, little-endian)
            frame_count = read_uint32_le(f.read(4))
            puts "  frame_count: #{frame_count}"
            
            # Skip reserved (12 bytes)
            f.read(12)
            
            @header = {
                magic: magic,
                version: version,
                entry_count: entry_count,
                frame_count: frame_count
            }
            @pos_init = 32;

            #check INIT frame
            pos = @pos_init
            while pos < (@header[:entry_count]+1) * ApuRegLog::ENTRY_SIZE
                #puts "check init frame pos=#{pos}"
                entry = read_entry_all(f,pos)
                if entry[:event_type] == ApuRegLog::APU_EVENT_WRITE || entry[:event_type] == ApuRegLog::APU_EVENT_INIT_START
                    #puts "frame:#{entry[:frame_number]}, addr:#{entry[:addr]}, data:#{entry[:data]}, event_type:#{entry[:event_type]} "
                elsif entry[:event_type] == ApuRegLog::APU_EVENT_INIT_END
                    @pos_play = pos + 12
                    break
                elsif entry[:event_type] == ApuRegLog::APU_EVENT_PLAY_START
                    @pos_play = f.tell
                    break
                else #PLAY_END
                    @pos_play = nil
                    puts "unexpected type #{entry[:event_type] }"
                    break
                end
                pos += ApuRegLog::ENTRY_SIZE 
            end
            #puts "@pos_play : #{@pos_play}"
        end #file close

        @current_frame = 0
        @current_pos = @pos_init
        @file = nil;
    end

    def read_entry_all(f,pos)
        time = read_uint32_le(f.read(4))
        addr = read_uint16_le(f.read(2))
        data = f.getbyte
        event_type = f.getbyte
        frame_number = read_uint32_le(f.read(4))
        
        {
            time: time,
            addr: addr,
            data: data,
            event_type: event_type,
            frame_number: frame_number
        }
    end

    def reset_to_init
        @current_frame = 0;
        if @file == nil
            #puts "open file #{@path}"
            @file = File.open(@path, "r")
        end
        @current_pos = @pos_init
        @file.seek(@current_pos, 0)
    end

    def restart_to_play
        @current_frame = 1;
        @current_pos = @pos_play
        if @file == nil
            @file = File.open(@path, "r")
        end
        @file.seek(@current_pos, 0)
    end

    def pop_entries_from_frame
        #puts "pop_entries_from_frame @current_pos=#{@current_pos}"

        pos = @current_pos;
        while pos < (@header[:entry_count]+1) * ApuRegLog::ENTRY_SIZE

            @file.read(4) # time is not used
            addr = read_uint16_le(@file.read(2))
            data = @file.getbyte
            event_type = @file.getbyte
            fno = read_uint32_le(@file.read(4)) #frame number

            #puts "fno:#{fno} pos:#{pos} < max:#{(@header[:entry_count]+1) * ApuRegLog::ENTRY_SIZE}, event_type:#{event_type}"

            if event_type == ApuRegLog::APU_EVENT_WRITE
                yield addr, data
            elsif event_type == ApuRegLog::APU_EVENT_PLAY_START
                #no write data, skip this frame
                @current_pos = @file.tell
                @current_frame = fno
                break
            end

            pos += ApuRegLog::ENTRY_SIZE
        end
    end
    
    def read_uint32_le(bytes)
        #return 0 if bytes.nil? || bytes.length < 4
        bytes.getbyte(0) | (bytes.getbyte(1) << 8) | (bytes.getbyte(2) << 16) | (bytes.getbyte(3) << 24)
    end
    
    def read_uint16_le(bytes)
        #return 0 if bytes.nil? || bytes.length < 2
        bytes.getbyte(0) | (bytes.getbyte(1) << 8)
    end
        
end

# NES APU Player
class MusicPlayer
    def initialize(mod, score)
        @sound_mod = mod
        @score = score
    end
    
    def play_init
        #puts "frame end = #{@score.header[:frame_count]}"

        puts "\nExecuting INIT sequence..."
        #puts "reset APU"
        @sound_mod.reset
        #puts "reset_to_init Score"
        @score.reset_to_init

        #puts "pop_entries_from_frame"
        @score.pop_entries_from_frame do |addr,data| #first frame is INIT
            @sound_mod.write_reg(addr, data)
        end
        @score.restart_to_play
        puts "Done"
        true
    end
    
    def play_loop(loop_flag)
        puts "\nStarting playback..."
        played_frame = 0
        frame_end = @score.header[:frame_count]

        loop do
            t1 = Machine.get_hwcount

            @score.pop_entries_from_frame do |addr,data| 
                @sound_mod.write_reg(addr, data)
            end
            played_frame += 1

            # Status every second (60 frames)
            # if played_frame % 60 == 0
            #     puts "Current Frame: #{@score.current_frame}"
            # end

            if @score.current_frame >= frame_end - 1
                if loop_flag
                    puts "restart_to_play"
                    @score.restart_to_play
                else
                    break
                end
            end
            
            consumed_time_ms = Machine.get_hwcount - t1
            # puts "consumed_time_ms:#{consumed_time_ms}"

            # wait ~16.67ms for 60Hz
            #ts1 = Machine.get_hwcount
            if consumed_time_ms < 16
                Machine.vtaskdelay(16 - consumed_time_ms + 1)
            end
            #ts2 = Machine.get_hwcount
            #puts "sleep time:#{ts2-ts1}"
        end
        puts "Playback stopped after #{played_frame} frames"
        true
    end
end

# Main execution
begin
    if ARGV.size == 0
        puts "usage"
        return
    end

    filename = ARGV[0]+".reglog"
    if not File.exist?(filename)
        puts "please set reglog file"
        return
    end

    loop_flag = false
    if ARGV.size == 2
        loop_flag = true if ARGV[1] == "loop"
    end

    # Load NSF register control log file
    #puts "load reglog #{filename}"
    reg_log = ApuRegLog.new(filename)
    
    # Create APU I/F
    #puts "create APU mod obj"
    apu = NesApu.new()

    # Create player
    #puts "create player"
    player = MusicPlayer.new(apu, reg_log)
    
    # Play music
    #puts "play music start"
    player.play_init
    player.play_loop(loop_flag)

    # Stop audio
    apu.reset

    puts "play music done"
    
rescue => e
    puts "Error: #{e}"
    puts "Error occurred in main section"
end
