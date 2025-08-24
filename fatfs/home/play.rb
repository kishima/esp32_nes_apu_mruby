puts "NSF replay test"

# APU Event Types
APU_EVENT_WRITE = 0
APU_EVENT_INIT_START = 1
APU_EVENT_INIT_END = 2
APU_EVENT_PLAY_START = 3
APU_EVENT_PLAY_END = 4

# APU Register Log Reader
class ApuRegLog
    attr_reader :header, :entries
    
    def initialize(path)
        @entries = []
        puts "loading... #{path}"
        
        File.open(path, "rb") do |f|
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
            
            # Read entries (12 bytes each)
            entry_count.times do |i|
                # time (4 bytes)
                time = read_uint32_le(f.read(4))
                
                # addr (2 bytes)
                addr = read_uint16_le(f.read(2))
                
                # data (1 byte)
                data = f.read(1).ord
                
                # event_type (1 byte)
                event_type = f.read(1).ord
                
                # frame_number (4 bytes)
                frame_number = read_uint32_le(f.read(4))
                
                @entries << {
                    time: time,
                    addr: addr,
                    data: data,
                    event_type: event_type,
                    frame_number: frame_number
                }
                
                # Debug output for first few entries
                if i < 5
                    puts "Entry #{i}: addr=0x#{addr.to_s(16)} data=0x#{data.to_s(16)} event=#{event_type} frame=#{frame_number}"
                end
            end
        end
    end
    
    # Helper methods for reading binary data
    def read_uint32_le(bytes)
        return 0 if bytes.nil? || bytes.length < 4
        bytes.getbyte(0) | (bytes.getbyte(1) << 8) | (bytes.getbyte(2) << 16) | (bytes.getbyte(3) << 24)
    end
    
    def read_uint16_le(bytes)
        return 0 if bytes.nil? || bytes.length < 2
        bytes.getbyte(0) | (bytes.getbyte(1) << 8)
    end
    
    # Find INIT sequence
    def find_init_sequence
        init_start = -1
        init_end = -1
        
        @entries.each_with_index do |entry, i|
            if entry[:event_type] == APU_EVENT_INIT_START
                init_start = i
            elsif entry[:event_type] == APU_EVENT_INIT_END
                init_end = i
                break
            end
        end
        
        return nil if init_start < 0 || init_end < 0
        @entries[init_start..init_end]
    end
    
    # Find first PLAY sequence
    def find_play_sequence
        play_start = -1
        play_end = -1
        
        @entries.each_with_index do |entry, i|
            if entry[:event_type] == APU_EVENT_PLAY_START
                play_start = i
            elsif entry[:event_type] == APU_EVENT_PLAY_END && play_start >= 0
                play_end = i
                break
            end
        end
        
        return nil if play_start < 0 || play_end < 0
        @entries[play_start..play_end]
    end
end

# Test loading
begin
    reg_log = ApuRegLog.new("/home/apu_log_track0.bin")
    
    # Find INIT sequence
    init_seq = reg_log.find_init_sequence
    if init_seq
        puts "\nFound INIT sequence with #{init_seq.length} entries"
    end
    
    # Find PLAY sequence
    play_seq = reg_log.find_play_sequence
    if play_seq
        puts "Found PLAY sequence with #{play_seq.length} entries"
    end
    
rescue => e
    puts "Error: #{e}"
    # backtrace is not available in PicoRuby
    puts "Error occurred during APU log loading"
end

# NES APU Player
class ApuPlayer
    def initialize(apu, apu_log)
        @apu = apu
        @apu_log = apu_log
        @current_frame = 0
    end
    
    def play_init
        puts "\nExecuting INIT sequence..."
        @apu.reset
        
        init_seq = @apu_log.find_init_sequence
        return false unless init_seq
        
        init_seq.each do |entry|
            if entry[:event_type] == APU_EVENT_WRITE && entry[:addr] != 0xFFFF
                @apu.write_reg(entry[:addr], entry[:data])
            end
        end
        
        puts "INIT complete"
        true
    end
    
    def play_frame
        # Find entries for current frame
        frame_entries = []
        @apu_log.entries.each do |entry|
            if entry[:frame_number] == @current_frame && entry[:event_type] == APU_EVENT_WRITE
                frame_entries << entry
            end
        end
        
        # Apply register writes
        frame_entries.each do |entry|
            if entry[:addr] != 0xFFFF
                @apu.write_reg(entry[:addr], entry[:data])
            end
        end
        
        # Process audio
        samples = @apu.process
        
        @current_frame += 1
        samples
    end
    
    def play_loop(num_frames = nil)
        puts "\nStarting playback..."
        
        frame_count = 0
        loop do
            samples = play_frame
            
            # Simple timing - wait ~16.67ms for 60Hz
            # In real implementation, should use proper timing
            sleep_ms(16)
            
            frame_count += 1
            
            # Status every second (60 frames)
            if frame_count % 60 == 0
                puts "Frame: #{frame_count}, Samples: #{samples}"
            end
            
            # Check if we should stop
            if num_frames && frame_count >= num_frames
                break
            end
            
            # Loop back to beginning if we reach the end
            if @current_frame >= @apu_log.header[:frame_count]
                @current_frame = 1  # Skip frame 0 (INIT)
                puts "Looping..."
            end
        end
        
        puts "Playback stopped after #{frame_count} frames"
    end
end

# Main execution
begin
    # Load APU log
    reg_log = ApuRegLog.new("/home/apu_log_track0.bin")
    
    # Create APU I/F
    apu = NesApu.new()
    # Create player
    player = ApuPlayer.new(apu, reg_log)
    
    # Initialize APU
    if player.play_init
        # Play for 5 seconds (300 frames)
        player.play_loop(300)
    else
        puts "Failed to initialize APU"
    end
    
rescue => e
    puts "Error: #{e}"
    # backtrace is not available in PicoRuby
    puts "Error occurred during APU log loading"
end