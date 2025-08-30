require 'uart'

class Midi
  def initialize(unit:, txd_pin:, rxd_pin:, baudrate:)
    @uart = UART.new(unit: unit, txd_pin: txd_pin, rxd_pin: rxd_pin, baudrate: baudrate)
  end

  def read
    @uart.read
  end

  def read_bytes(n)
    @uart.read(n)
  end

  def getc
    @uart.read(1)
  end

  def bytes_available
    @uart.bytes_available
  end

  def parse_midi_message(bytes)
    return nil if bytes.nil? || bytes.empty?
    
    status = bytes[0]
    
    # Status byte (最上位ビットが1)
    if (status & 0x80) == 0
      return nil  # Not a status byte
    end
    
    msg_type = status & 0xF0
    channel = (status & 0x0F) + 1
    
    case msg_type
    when 0x80  # Note Off
      return "Note Off - Ch:#{channel} Note:#{bytes[1]} Velocity:#{bytes[2]}" if bytes.length >= 3
    when 0x90  # Note On
      return "Note On  - Ch:#{channel} Note:#{bytes[1]} Velocity:#{bytes[2]}" if bytes.length >= 3
    when 0xA0  # Polyphonic Aftertouch
      return "Poly AT  - Ch:#{channel} Note:#{bytes[1]} Pressure:#{bytes[2]}" if bytes.length >= 3
    when 0xB0  # Control Change
      return "Ctrl Chg - Ch:#{channel} Controller:#{bytes[1]} Value:#{bytes[2]}" if bytes.length >= 3
    when 0xC0  # Program Change
      return "Prog Chg - Ch:#{channel} Program:#{bytes[1]}" if bytes.length >= 2
    when 0xD0  # Channel Aftertouch
      return "Chan AT  - Ch:#{channel} Pressure:#{bytes[1]}" if bytes.length >= 2
    when 0xE0  # Pitch Bend
      return "PitchBnd - Ch:#{channel} Value:#{(bytes[2] << 7) | bytes[1]}" if bytes.length >= 3
    when 0xF0  # System messages
      case status
      when 0xF8
        return "Timing Clock"
      when 0xFA
        return "Start"
      when 0xFB
        return "Continue"
      when 0xFC
        return "Stop"
      when 0xFE
        return "Active Sensing"
      when 0xFF
        return "System Reset"
      else
        return "System Message: 0x#{status.to_s(16)}"
      end
    end
    
    return "Unknown: 0x#{status.to_s(16)}"
  end
end


begin
    midi = Midi.new(
        unit: "ESP32_UART1",
        txd_pin: -1, # -1 for unused pin
        rxd_pin: 32,
        baudrate: 31250  # 標準MIDIボーレート
        )

    puts "MIDI Monitor Started"
    puts "Baudrate: 31250"
    puts "Waiting for MIDI data..."
    puts "-" * 40

    message_buffer = []
    
    loop do
        if midi.bytes_available > 0
            byte = midi.getc
            if byte && byte.length > 0
                byte_val = byte.ord
                puts "val=#{byte_val}"
                
                # Status byte detected (MSB = 1)
                if (byte_val & 0x80) != 0
                    # Process previous message if exists
                    if message_buffer.length > 0
                        msg = midi.parse_midi_message(message_buffer)
                        puts msg if msg
                    end
                    # Start new message
                    message_buffer = [byte_val]
                else
                    # Data byte - add to current message
                    message_buffer << byte_val if message_buffer.length > 0
                    
                    # Check if message is complete
                    if message_buffer.length > 0
                        status = message_buffer[0] & 0xF0
                        expected_length = case status
                        when 0x80, 0x90, 0xA0, 0xB0, 0xE0
                            3
                        when 0xC0, 0xD0
                            2
                        when 0xF0
                            1  # System messages vary
                        else
                            3
                        end
                        
                        if message_buffer.length >= expected_length
                            msg = midi.parse_midi_message(message_buffer)
                            puts msg if msg
                            message_buffer = []
                        end
                    end
                end
            end
        else
            Machine.vtaskdelay(1)
        end
    end

rescue => e
    puts "Error: #{e}"
    puts "Error occurred in main section"
end