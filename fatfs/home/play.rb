puts "NSF replay test"

require "filesystem-fat"
require "nes_apu"
require "machine"

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
            puts "magic: #{magic.inspect}"
            
            # Read version (4 bytes, little-endian)
            version_bytes = f.read(4)
            version = read_uint32_le(version_bytes)
            puts "version: #{version}"
            
            # Read entry_count (4 bytes, little-endian)
            entry_count = read_uint32_le(f.read(4))
            puts "entry_count: #{entry_count}"
            
            # Read frame_count (4 bytes, little-endian)
            frame_count = read_uint32_le(f.read(4))
            puts "frame_count: #{frame_count}"
            
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
            (@pos_init .. (@header[:entry_count]+1)*ApuRegLog::ENTRY_SIZE).step(ApuRegLog::ENTRY_SIZE).each do |pos|
                puts "check init frame pos=#{pos}"
                entry = read_entry_all(f,pos)
                if entry[:event_type] == ApuRegLog::APU_EVENT_WRITE || entry[:event_type] == ApuRegLog::APU_EVENT_INIT_START
                    puts "#{entry[:frame_number]}: addr:#{entry[:addr]}, data:#{entry[:data]}, event_type:#{entry[:event_type]} "
                elsif entry[:event_type] == ApuRegLog::APU_EVENT_INIT_END
                    @pos_play = pos + 12
                    break
                elsif entry[:event_type] == ApuRegLog::APU_EVENT_PLAY_START
                    @pos_play = pos
                    break
                else #PLAY_END
                    @pos_play = nil
                    puts "unexpected type #{entry[:event_type] }"
                    break
                end
            end
            puts "@pos_play : #{@pos_play}"
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

    # def read_entry_write(f,pos)
    #     f.read(4) # time is not used
    #     addr = read_uint16_le(f.read(2))
    #     data = f.getbyte
    #     event_type = f.getbyte
    #     if event_type == APU_EVENT_WRITE
    #         return addr, data, event_type
    #     end
    #     nil
    # end

    def reset
        @current_frame = 0;
        @file.close if @file
    end

    def restart_to_play
        @current_frame = 1;
        @current_pos = @pos_play
        if @file == nil
            @file = File.open(@path, "r")
        end
        @file.seek(@current_pos, IO::SEEK_SET)
    end

    def pop_entries_from_frame
        puts "pop_entries_from_frame @current_pos=#{@current_pos}"

        if @file == nil
            @file = File.open(@path, "r")
        end

        is_write_fetched = false

        @current_pos.step(ApuRegLog::ENTRY_SIZE) do |pos|
            val = @file.read(4) # time is not used
            if val == nil #end
                if @file.eof?
                    # Loop back to beginning if we reach the end
                    restart_to_play
                else
                    puts "read error!"
                end
                break
            end
            addr = read_uint16_le(@file.read(2))
            data = @file.getbyte
            event_type = @file.getbyte
            if event_type == ApuRegLog::APU_EVENT_WRITE
                is_write_fetched = true
                yield addr, data
            elsif event_type == ApuRegLog::APU_EVENT_PLAY_START
                if is_write_fetched
                    #new frame
                    @current_pos = @file.tell
                    break
                end
            end
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
        puts "\nExecuting INIT sequence..."
        @sound_mod.reset
        @score.reset

        @score.pop_entries_from_frame do |addr,data| #first frame is INIT
            puts "addr:#{addr} data:#{data}"
            @sound_mod.write_reg(addr, data)
        end
        true
    end
    
    def play_loop(max_frames = nil)
        puts "\nStarting playback..."
        played_frame = 0

        loop do
            t1 = Machine.get_hwcount

            @score.pop_entries_from_frame do |addr,data|                
                @sound_mod.write_reg(addr, data)
            end
            played_frame += 1

            # Status every second (60 frames)
            if played_frame % 60 == 0
                puts "Current Frame: #{@score.current_frame}"
            end
            
            # Check if we should stop
            if max_frames && played_frame >= max_frames
                break
            end
            
            consumed_time_ms = Machine.get_hwcount - t1
            # wait ~16.67ms for 60Hz
            if consumed_time_ms < 16
                sleep_ms(16 - consumed_time_ms)
            end
        end
        puts "Playback stopped after #{played_frame} frames"
        true
    end
end

# Main execution
begin
    # Load NSF register control log file
    reg_log = ApuRegLog.new("/home/dq.bin")
    
    # Create APU I/F
    apu = NesApu.new()

    # Create player
    player = MusicPlayer.new(apu, reg_log)
    
    # Play music
    if player.play_init
        player.play_loop(60*60)
    end
    
rescue => e
    puts "Error: #{e}"
    puts "Error occurred in main section"
end