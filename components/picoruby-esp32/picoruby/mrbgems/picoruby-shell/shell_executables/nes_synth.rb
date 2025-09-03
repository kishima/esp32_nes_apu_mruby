require 'uart'
require 'nes-apu'

class MidiApu
  # MIDI note to NES APU frequency conversion
  # NES APU timer = 1789773 / (16 * frequency) - 1
  MIDI_TO_FREQ = {
    # Octave 2
    36 => 65.41,   # C2
    37 => 69.30,   # C#2
    38 => 73.42,   # D2
    39 => 77.78,   # D#2
    40 => 82.41,   # E2
    41 => 87.31,   # F2
    42 => 92.50,   # F#2
    43 => 98.00,   # G2
    44 => 103.83,  # G#2
    45 => 110.00,  # A2
    46 => 116.54,  # A#2
    47 => 123.47,  # B2
    # Octave 3
    48 => 130.81,  # C3
    49 => 138.59,  # C#3
    50 => 146.83,  # D3
    51 => 155.56,  # D#3
    52 => 164.81,  # E3
    53 => 174.61,  # F3
    54 => 185.00,  # F#3
    55 => 196.00,  # G3
    56 => 207.65,  # G#3
    57 => 220.00,  # A3
    58 => 233.08,  # A#3
    59 => 246.94,  # B3
    # Octave 4
    60 => 261.63,  # C4 (Middle C)
    61 => 277.18,  # C#4
    62 => 293.66,  # D4
    63 => 311.13,  # D#4
    64 => 329.63,  # E4
    65 => 349.23,  # F4
    66 => 369.99,  # F#4
    67 => 392.00,  # G4
    68 => 415.30,  # G#4
    69 => 440.00,  # A4
    70 => 466.16,  # A#4
    71 => 493.88,  # B4
    # Octave 5
    72 => 523.25,  # C5
    73 => 554.37,  # C#5
    74 => 587.33,  # D5
    75 => 622.25,  # D#5
    76 => 659.25,  # E5
    77 => 698.46,  # F5
    78 => 739.99,  # F#5
    79 => 783.99,  # G5
    80 => 830.61,  # G#5
    81 => 880.00,  # A5
    82 => 932.33,  # A#5
    83 => 987.77,  # B5
    84 => 1046.50  # C6
  }

  def initialize(unit:, txd_pin:, rxd_pin:, baudrate:)
    @uart = UART.new(unit: unit, txd_pin: txd_pin, rxd_pin: rxd_pin, baudrate: baudrate)
    @apu = NesApu.new()
    @active_notes = {}  # Track active notes for proper Note Off handling
    
    # Initialize APU
    @apu.reset
    @apu.write_reg(0x4015, 0x00)  # Disable all channels
    @apu.write_reg(0x4001, 0x00)  # Disable sweep
    @apu.write_reg(0x4017, 0x40)  # Frame counter 4-step mode
  end

  def read
    @uart.read(1)
  end

  def bytes_available
    @uart.bytes_available
  end

  def midi_note_to_timer(note)
    freq = MIDI_TO_FREQ[note]
    return nil if freq.nil?
    
    # NES APU timer calculation
    timer = (1789773.0 / (16.0 * freq) - 1).to_i
    timer = 7 if timer < 8  # Minimum timer value is 8
    timer = 0x7FF if timer > 0x7FF  # Maximum timer value is 0x7FF
    timer
  end

  def velocity_to_volume(velocity)
    # Convert MIDI velocity (0-127) to APU volume (0-15)
    (velocity >> 3) & 0x0F
  end

  def note_on(channel, note, velocity)
    volume = velocity_to_volume(velocity)
    
    # Store active note
    @active_notes[channel] = note
    
    # Channel 10 is drums - use noise channel
    if channel == 10
      note_on_noise(note, volume)
    else
      # Regular notes - use square wave
      timer = midi_note_to_timer(note)
      return if timer.nil?
      
      # Square wave 1 settings
      # 0x4000: Duty cycle 50% (0x80) + length counter halt (0x20) + disable envelope (0x10) + volume
      @apu.write_reg(0x4000, 0xB0 | volume)
      
      # 0x4002: Timer low byte
      @apu.write_reg(0x4002, timer & 0xFF)
      
      # 0x4003: Timer high 3 bits + length counter disabled (0x00)
      @apu.write_reg(0x4003, 0x00 | ((timer >> 8) & 0x07))
      
      # Enable square wave 1
      @apu.write_reg(0x4015, 0x01)
    end
  end

  def note_on_noise(note, volume)
    # Map MIDI drum notes to noise settings
    # $400C: --LC VVVV (Length counter halt, constant volume, volume)
    # $400E: L--- PPPP (Loop noise, period)
    # $400F: LLLLL --- (Length counter load)
    
    # Different noise periods for different drum sounds
    period = case note
    when 36      # C1 - Bass Drum (low frequency noise)
      0x0F     # Lowest frequency
    when 38      # D1 - Snare Drum
      0x04     # Higher frequency for snare
    when 40..41  # E1-F1 - Electric Snare
      0x02     # Even higher frequency
    when 42      # F#1 - Closed Hi-Hat
      0x00     # Highest frequency
    else
      0x08     # Default medium frequency
    end
    
    # Use short mode (metallic sound) for hi-hats, long mode for drums
    mode = (note >= 42) ? 0x80 : 0x00  # Short mode for hi-hats
    
    # $400C: Length counter halt + constant volume + volume
    @apu.write_reg(0x400C, 0x30 | volume)
    
    # $400E: Mode (short/long) + period  
    @apu.write_reg(0x400E, mode | period)
    
    # $400F: Length counter (short burst for drums)
    @apu.write_reg(0x400F, 0x40)
    
    # Enable noise channel
    @apu.write_reg(0x4015, 0x08)
  end

  def note_off(channel, note)
    # Only turn off if this note is currently playing
    if @active_notes[channel] == note
      if channel == 10
        # Disable noise channel
        @apu.write_reg(0x4015, 0x00)
      else
        # Disable square wave 1
        @apu.write_reg(0x4015, 0x00)
      end
      @active_notes.delete(channel)
    end
  end

  def parse_midi_message(bytes)
    return nil if bytes.nil? || bytes.empty?
    
    status = bytes[0]
    
    # Status byte check
    if (status & 0x80) == 0
      return nil
    end
    
    msg_type = status & 0xF0
    channel = (status & 0x0F) + 1
    
    case msg_type
    when 0x80  # Note Off
      if bytes.length >= 3
        note_off(channel, bytes[1])
        return "Note Off - Ch:#{channel} Note:#{bytes[1]}"
      end
    when 0x90  # Note On
      if bytes.length >= 3
        if bytes[2] == 0  # Velocity 0 means Note Off
          note_off(channel, bytes[1])
          return "Note Off - Ch:#{channel} Note:#{bytes[1]} (vel=0)"
        else
          note_on(channel, bytes[1], bytes[2])
          return "Note On  - Ch:#{channel} Note:#{bytes[1]} Velocity:#{bytes[2]}"
        end
      end
    end
    
    return nil
  end

  def process_audio
    #@apu.process(0)  # Internal audio processing
    @apu.process(1)  # External audio processing
  end
end

# Main execution
begin
  midi_apu = MidiApu.new(
    unit: "ESP32_UART1",
    txd_pin: -1,
    rxd_pin: 32,
    baudrate: 31250
  )

  puts "MIDI to NES APU Started"
  puts "Using Square Wave Channel 1"
  puts "Waiting for MIDI input..."
  puts "-" * 40

  message_buffer = []
  
  loop do
    t1 = Machine.get_hwcount

    # Process MIDI input
    if midi_apu.bytes_available > 0
      byte = midi_apu.read
      if byte && byte.length > 0
        byte_val = byte.ord
        
        # Status byte detected
        if (byte_val & 0x80) != 0
          # Process previous message if exists
          if message_buffer.length > 0
            msg = midi_apu.parse_midi_message(message_buffer)
            puts msg if msg
          end
          # Start new message
          message_buffer = [byte_val]
        else
          # Data byte
          message_buffer << byte_val if message_buffer.length > 0
          
          # Check if message is complete
          if message_buffer.length > 0
            status = message_buffer[0] & 0xF0
            expected_length = case status
            when 0x80, 0x90  # Note Off/On
              3
            else
              3
            end
            
            if message_buffer.length >= expected_length
              msg = midi_apu.parse_midi_message(message_buffer)
              puts msg if msg
              message_buffer = []
            end
          end
        end
      end
    end
    
    # Process audio periodically
    midi_apu.process_audio

    consumed_time_ms = Machine.get_hwcount - t1
    # wait ~16.67ms for 60Hz
    #ts1 = Machine.get_hwcount
    if consumed_time_ms < 16
        Machine.vtaskdelay(16 - consumed_time_ms + 1)
    end
  end

rescue => e
  puts "Error: #{e}"
  puts "Error occurred in main section"
end