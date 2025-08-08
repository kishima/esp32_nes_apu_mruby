#!/usr/bin/env ruby
# NES ROM Dumper and 6502 Disassembler
# Parses NES ROM files and displays machine code with assembly disassembly

require 'optparse'

class NES6502Disassembler
  # 6502 addressing modes
  ADDRESSING_MODES = {
    :implicit => 1,
    :accumulator => 1,
    :immediate => 2,
    :zero_page => 2,
    :zero_page_x => 2,
    :zero_page_y => 2,
    :relative => 2,
    :absolute => 3,
    :absolute_x => 3,
    :absolute_y => 3,
    :indirect => 3,
    :indexed_indirect => 2,  # (zp,X)
    :indirect_indexed => 2   # (zp),Y
  }

  # 6502 instruction set with opcodes, mnemonics, and addressing modes
  INSTRUCTIONS = {
    # ADC - Add with Carry
    0x69 => { mnemonic: "ADC", mode: :immediate },
    0x65 => { mnemonic: "ADC", mode: :zero_page },
    0x75 => { mnemonic: "ADC", mode: :zero_page_x },
    0x6D => { mnemonic: "ADC", mode: :absolute },
    0x7D => { mnemonic: "ADC", mode: :absolute_x },
    0x79 => { mnemonic: "ADC", mode: :absolute_y },
    0x61 => { mnemonic: "ADC", mode: :indexed_indirect },
    0x71 => { mnemonic: "ADC", mode: :indirect_indexed },

    # AND - Logical AND
    0x29 => { mnemonic: "AND", mode: :immediate },
    0x25 => { mnemonic: "AND", mode: :zero_page },
    0x35 => { mnemonic: "AND", mode: :zero_page_x },
    0x2D => { mnemonic: "AND", mode: :absolute },
    0x3D => { mnemonic: "AND", mode: :absolute_x },
    0x39 => { mnemonic: "AND", mode: :absolute_y },
    0x21 => { mnemonic: "AND", mode: :indexed_indirect },
    0x31 => { mnemonic: "AND", mode: :indirect_indexed },

    # ASL - Arithmetic Shift Left
    0x0A => { mnemonic: "ASL", mode: :accumulator },
    0x06 => { mnemonic: "ASL", mode: :zero_page },
    0x16 => { mnemonic: "ASL", mode: :zero_page_x },
    0x0E => { mnemonic: "ASL", mode: :absolute },
    0x1E => { mnemonic: "ASL", mode: :absolute_x },

    # Branch Instructions
    0x90 => { mnemonic: "BCC", mode: :relative },
    0xB0 => { mnemonic: "BCS", mode: :relative },
    0xF0 => { mnemonic: "BEQ", mode: :relative },
    0x30 => { mnemonic: "BMI", mode: :relative },
    0xD0 => { mnemonic: "BNE", mode: :relative },
    0x10 => { mnemonic: "BPL", mode: :relative },
    0x50 => { mnemonic: "BVC", mode: :relative },
    0x70 => { mnemonic: "BVS", mode: :relative },

    # BIT - Bit Test
    0x24 => { mnemonic: "BIT", mode: :zero_page },
    0x2C => { mnemonic: "BIT", mode: :absolute },

    # BRK - Break
    0x00 => { mnemonic: "BRK", mode: :implicit },

    # Clear/Set Instructions
    0x18 => { mnemonic: "CLC", mode: :implicit },
    0xD8 => { mnemonic: "CLD", mode: :implicit },
    0x58 => { mnemonic: "CLI", mode: :implicit },
    0xB8 => { mnemonic: "CLV", mode: :implicit },
    0x38 => { mnemonic: "SEC", mode: :implicit },
    0xF8 => { mnemonic: "SED", mode: :implicit },
    0x78 => { mnemonic: "SEI", mode: :implicit },

    # CMP - Compare Accumulator
    0xC9 => { mnemonic: "CMP", mode: :immediate },
    0xC5 => { mnemonic: "CMP", mode: :zero_page },
    0xD5 => { mnemonic: "CMP", mode: :zero_page_x },
    0xCD => { mnemonic: "CMP", mode: :absolute },
    0xDD => { mnemonic: "CMP", mode: :absolute_x },
    0xD9 => { mnemonic: "CMP", mode: :absolute_y },
    0xC1 => { mnemonic: "CMP", mode: :indexed_indirect },
    0xD1 => { mnemonic: "CMP", mode: :indirect_indexed },

    # CPX - Compare X Register
    0xE0 => { mnemonic: "CPX", mode: :immediate },
    0xE4 => { mnemonic: "CPX", mode: :zero_page },
    0xEC => { mnemonic: "CPX", mode: :absolute },

    # CPY - Compare Y Register
    0xC0 => { mnemonic: "CPY", mode: :immediate },
    0xC4 => { mnemonic: "CPY", mode: :zero_page },
    0xCC => { mnemonic: "CPY", mode: :absolute },

    # DEC - Decrement
    0xC6 => { mnemonic: "DEC", mode: :zero_page },
    0xD6 => { mnemonic: "DEC", mode: :zero_page_x },
    0xCE => { mnemonic: "DEC", mode: :absolute },
    0xDE => { mnemonic: "DEC", mode: :absolute_x },
    0xCA => { mnemonic: "DEX", mode: :implicit },
    0x88 => { mnemonic: "DEY", mode: :implicit },

    # EOR - Exclusive OR
    0x49 => { mnemonic: "EOR", mode: :immediate },
    0x45 => { mnemonic: "EOR", mode: :zero_page },
    0x55 => { mnemonic: "EOR", mode: :zero_page_x },
    0x4D => { mnemonic: "EOR", mode: :absolute },
    0x5D => { mnemonic: "EOR", mode: :absolute_x },
    0x59 => { mnemonic: "EOR", mode: :absolute_y },
    0x41 => { mnemonic: "EOR", mode: :indexed_indirect },
    0x51 => { mnemonic: "EOR", mode: :indirect_indexed },

    # INC - Increment
    0xE6 => { mnemonic: "INC", mode: :zero_page },
    0xF6 => { mnemonic: "INC", mode: :zero_page_x },
    0xEE => { mnemonic: "INC", mode: :absolute },
    0xFE => { mnemonic: "INC", mode: :absolute_x },
    0xE8 => { mnemonic: "INX", mode: :implicit },
    0xC8 => { mnemonic: "INY", mode: :implicit },

    # JMP - Jump
    0x4C => { mnemonic: "JMP", mode: :absolute },
    0x6C => { mnemonic: "JMP", mode: :indirect },

    # JSR - Jump to Subroutine
    0x20 => { mnemonic: "JSR", mode: :absolute },

    # LDA - Load Accumulator
    0xA9 => { mnemonic: "LDA", mode: :immediate },
    0xA5 => { mnemonic: "LDA", mode: :zero_page },
    0xB5 => { mnemonic: "LDA", mode: :zero_page_x },
    0xAD => { mnemonic: "LDA", mode: :absolute },
    0xBD => { mnemonic: "LDA", mode: :absolute_x },
    0xB9 => { mnemonic: "LDA", mode: :absolute_y },
    0xA1 => { mnemonic: "LDA", mode: :indexed_indirect },
    0xB1 => { mnemonic: "LDA", mode: :indirect_indexed },

    # LDX - Load X Register
    0xA2 => { mnemonic: "LDX", mode: :immediate },
    0xA6 => { mnemonic: "LDX", mode: :zero_page },
    0xB6 => { mnemonic: "LDX", mode: :zero_page_y },
    0xAE => { mnemonic: "LDX", mode: :absolute },
    0xBE => { mnemonic: "LDX", mode: :absolute_y },

    # LDY - Load Y Register
    0xA0 => { mnemonic: "LDY", mode: :immediate },
    0xA4 => { mnemonic: "LDY", mode: :zero_page },
    0xB4 => { mnemonic: "LDY", mode: :zero_page_x },
    0xAC => { mnemonic: "LDY", mode: :absolute },
    0xBC => { mnemonic: "LDY", mode: :absolute_x },

    # LSR - Logical Shift Right
    0x4A => { mnemonic: "LSR", mode: :accumulator },
    0x46 => { mnemonic: "LSR", mode: :zero_page },
    0x56 => { mnemonic: "LSR", mode: :zero_page_x },
    0x4E => { mnemonic: "LSR", mode: :absolute },
    0x5E => { mnemonic: "LSR", mode: :absolute_x },

    # NOP - No Operation
    0xEA => { mnemonic: "NOP", mode: :implicit },

    # ORA - Logical OR
    0x09 => { mnemonic: "ORA", mode: :immediate },
    0x05 => { mnemonic: "ORA", mode: :zero_page },
    0x15 => { mnemonic: "ORA", mode: :zero_page_x },
    0x0D => { mnemonic: "ORA", mode: :absolute },
    0x1D => { mnemonic: "ORA", mode: :absolute_x },
    0x19 => { mnemonic: "ORA", mode: :absolute_y },
    0x01 => { mnemonic: "ORA", mode: :indexed_indirect },
    0x11 => { mnemonic: "ORA", mode: :indirect_indexed },

    # Stack Instructions
    0x48 => { mnemonic: "PHA", mode: :implicit },
    0x08 => { mnemonic: "PHP", mode: :implicit },
    0x68 => { mnemonic: "PLA", mode: :implicit },
    0x28 => { mnemonic: "PLP", mode: :implicit },

    # ROL - Rotate Left
    0x2A => { mnemonic: "ROL", mode: :accumulator },
    0x26 => { mnemonic: "ROL", mode: :zero_page },
    0x36 => { mnemonic: "ROL", mode: :zero_page_x },
    0x2E => { mnemonic: "ROL", mode: :absolute },
    0x3E => { mnemonic: "ROL", mode: :absolute_x },

    # ROR - Rotate Right
    0x6A => { mnemonic: "ROR", mode: :accumulator },
    0x66 => { mnemonic: "ROR", mode: :zero_page },
    0x76 => { mnemonic: "ROR", mode: :zero_page_x },
    0x6E => { mnemonic: "ROR", mode: :absolute },
    0x7E => { mnemonic: "ROR", mode: :absolute_x },

    # RTI - Return from Interrupt
    0x40 => { mnemonic: "RTI", mode: :implicit },

    # RTS - Return from Subroutine
    0x60 => { mnemonic: "RTS", mode: :implicit },

    # SBC - Subtract with Carry
    0xE9 => { mnemonic: "SBC", mode: :immediate },
    0xE5 => { mnemonic: "SBC", mode: :zero_page },
    0xF5 => { mnemonic: "SBC", mode: :zero_page_x },
    0xED => { mnemonic: "SBC", mode: :absolute },
    0xFD => { mnemonic: "SBC", mode: :absolute_x },
    0xF9 => { mnemonic: "SBC", mode: :absolute_y },
    0xE1 => { mnemonic: "SBC", mode: :indexed_indirect },
    0xF1 => { mnemonic: "SBC", mode: :indirect_indexed },

    # STA - Store Accumulator
    0x85 => { mnemonic: "STA", mode: :zero_page },
    0x95 => { mnemonic: "STA", mode: :zero_page_x },
    0x8D => { mnemonic: "STA", mode: :absolute },
    0x9D => { mnemonic: "STA", mode: :absolute_x },
    0x99 => { mnemonic: "STA", mode: :absolute_y },
    0x81 => { mnemonic: "STA", mode: :indexed_indirect },
    0x91 => { mnemonic: "STA", mode: :indirect_indexed },

    # STX - Store X Register
    0x86 => { mnemonic: "STX", mode: :zero_page },
    0x96 => { mnemonic: "STX", mode: :zero_page_y },
    0x8E => { mnemonic: "STX", mode: :absolute },

    # STY - Store Y Register
    0x84 => { mnemonic: "STY", mode: :zero_page },
    0x94 => { mnemonic: "STY", mode: :zero_page_x },
    0x8C => { mnemonic: "STY", mode: :absolute },

    # Transfer Instructions
    0xAA => { mnemonic: "TAX", mode: :implicit },
    0xA8 => { mnemonic: "TAY", mode: :implicit },
    0xBA => { mnemonic: "TSX", mode: :implicit },
    0x8A => { mnemonic: "TXA", mode: :implicit },
    0x9A => { mnemonic: "TXS", mode: :implicit },
    0x98 => { mnemonic: "TYA", mode: :implicit }
  }

  def initialize
  end

  def format_operand(mode, bytes, address)
    case mode
    when :implicit, :accumulator
      ""
    when :immediate
      "#$%02X" % bytes[1]
    when :zero_page
      "$%02X" % bytes[1]
    when :zero_page_x
      "$%02X,X" % bytes[1]
    when :zero_page_y
      "$%02X,Y" % bytes[1]
    when :absolute
      "$%04X" % (bytes[1] | (bytes[2] << 8))
    when :absolute_x
      "$%04X,X" % (bytes[1] | (bytes[2] << 8))
    when :absolute_y
      "$%04X,Y" % (bytes[1] | (bytes[2] << 8))
    when :relative
      target = address + bytes[1] + 2
      target += 256 if bytes[1] > 127  # Handle negative offset
      target -= 256 if target > 0xFFFF
      "$%04X" % target
    when :indirect
      "$(%04X)" % (bytes[1] | (bytes[2] << 8))
    when :indexed_indirect
      "($%02X,X)" % bytes[1]
    when :indirect_indexed
      "($%02X),Y" % bytes[1]
    else
      "???"
    end
  end

  def disassemble_instruction(data, address)
    return nil if data.empty?
    
    opcode = data[0]
    # Convert to integer if it's a string byte
    opcode = opcode.ord if opcode.is_a?(String)
    instruction = INSTRUCTIONS[opcode]
    
    unless instruction
      return {
        bytes: [opcode],
        hex: "%02X" % opcode,
        mnemonic: "???",
        operand: "",
        size: 1
      }
    end

    size = ADDRESSING_MODES[instruction[:mode]]
    bytes = data[0, size] || [opcode]
    
    # Convert string bytes to integers
    bytes = bytes.bytes if bytes.is_a?(String)
    
    # Handle case where we don't have enough bytes
    while bytes.length < size
      bytes << 0
    end

    operand = format_operand(instruction[:mode], bytes, address)
    hex_string = bytes.map { |b| "%02X" % b }.join(" ")

    {
      bytes: bytes,
      hex: hex_string,
      mnemonic: instruction[:mnemonic],
      operand: operand,
      size: size
    }
  end
end

class NESRomParser
  attr_reader :header, :prg_rom, :chr_rom, :trainer

  INES_MAGIC = "NES\x1A".freeze

  def initialize(filename)
    @filename = filename
    @data = nil
    @header = {}
    @prg_rom = nil
    @chr_rom = nil
    @trainer = nil
  end

  def parse
    File.open(@filename, 'rb') do |file|
      @data = file.read
    end

    return false unless parse_header
    parse_data
    true
  end

  private

  def parse_header
    return false if @data.length < 16
    return false unless @data[0, 4] == INES_MAGIC

    # Convert bytes to integers for calculations
    bytes = @data.bytes
    
    @header = {
      magic: @data[0, 4],
      prg_rom_size: bytes[4] * 16 * 1024,  # 16KB units
      chr_rom_size: bytes[5] * 8 * 1024,   # 8KB units
      flags6: bytes[6],
      flags7: bytes[7],
      prg_ram_size: bytes[8] * 8 * 1024,   # 8KB units, 0 means 8KB for compatibility
      flags9: bytes[9],
      flags10: bytes[10],
      padding: @data[11, 5]
    }

    # Parse flags
    @header[:mapper] = ((@header[:flags7] & 0xF0) | (@header[:flags6] >> 4))
    @header[:mirroring] = @header[:flags6] & 0x01 == 0 ? :horizontal : :vertical
    @header[:battery_backed] = (@header[:flags6] & 0x02) != 0
    @header[:trainer_present] = (@header[:flags6] & 0x04) != 0
    @header[:four_screen] = (@header[:flags6] & 0x08) != 0
    @header[:vs_unisystem] = (@header[:flags7] & 0x01) != 0
    @header[:playchoice10] = (@header[:flags7] & 0x02) != 0

    # Set default PRG RAM size if 0
    @header[:prg_ram_size] = 8 * 1024 if @header[:prg_ram_size] == 0

    true
  end

  def parse_data
    offset = 16  # Header size

    # Trainer (512 bytes if present)
    if @header[:trainer_present]
      @trainer = @data[offset, 512]
      offset += 512
    end

    # PRG ROM
    if @header[:prg_rom_size] > 0
      @prg_rom = @data[offset, @header[:prg_rom_size]]
      offset += @header[:prg_rom_size]
    end

    # CHR ROM
    if @header[:chr_rom_size] > 0
      @chr_rom = @data[offset, @header[:chr_rom_size]]
    end
  end

  public

  def print_header
    puts "=" * 60
    puts "NES ROM Header Information"
    puts "=" * 60
    puts "File: #{@filename}"
    puts "Magic: #{@header[:magic].unpack('H*')[0].upcase}"
    puts "PRG ROM Size: #{@header[:prg_rom_size]} bytes (#{@header[:prg_rom_size] / 1024}KB)"
    puts "CHR ROM Size: #{@header[:chr_rom_size]} bytes (#{@header[:chr_rom_size] / 1024}KB)" 
    puts "PRG RAM Size: #{@header[:prg_ram_size]} bytes (#{@header[:prg_ram_size] / 1024}KB)"
    puts "Mapper: #{@header[:mapper]}"
    puts "Mirroring: #{@header[:mirroring].to_s.capitalize}"
    puts "Battery Backed: #{@header[:battery_backed] ? 'Yes' : 'No'}"
    puts "Trainer Present: #{@header[:trainer_present] ? 'Yes' : 'No'}"
    puts "Four Screen: #{@header[:four_screen] ? 'Yes' : 'No'}"
    puts "VS UniSystem: #{@header[:vs_unisystem] ? 'Yes' : 'No'}"
    puts "PlayChoice-10: #{@header[:playchoice10] ? 'Yes' : 'No'}"
    puts
  end

  def disassemble_prg_rom(start_offset = 0, length = nil, base_address = 0x8000)
    return unless @prg_rom

    disassembler = NES6502Disassembler.new
    
    # Default to entire PRG ROM if no length specified
    length ||= @prg_rom.length - start_offset
    length = [@prg_rom.length - start_offset, length].min

    puts "=" * 80
    puts "PRG ROM Disassembly (Offset: $%04X, Length: %d bytes, Base: $%04X)" % [start_offset, length, base_address]
    puts "=" * 80
    puts "Address  | Hex Code     | Assembly"
    puts "-" * 80

    offset = start_offset
    address = base_address

    while offset < start_offset + length && offset < @prg_rom.length
      data = @prg_rom[offset..-1]
      break if data.empty?

      instruction = disassembler.disassemble_instruction(data, address)
      
      # Format the output
      hex_padding = " " * (12 - instruction[:hex].length)
      operand_part = instruction[:operand].empty? ? "" : " #{instruction[:operand]}"
      
      puts "$%04X    | %-12s | %s%s" % [
        address,
        instruction[:hex],
        instruction[:mnemonic],
        operand_part
      ]

      offset += instruction[:size]
      address += instruction[:size]
    end
    puts
  end

  def hex_dump(data, title, start_address = 0, max_bytes = 256)
    return unless data && !data.empty?

    puts "=" * 60
    puts "#{title} (#{data.length} bytes)"
    puts "=" * 60

    bytes_to_show = [data.length, max_bytes].min
    
    (0...bytes_to_show).step(16) do |i|
      # Address
      address = start_address + i
      print "$%04X: " % address

      # Hex bytes
      line_bytes = data[i, 16]
      hex_part = line_bytes.unpack('C*').map { |b| "%02X" % b }.join(' ')
      print "%-47s " % hex_part

      # ASCII part
      ascii_part = line_bytes.gsub(/[^\x20-\x7E]/, '.')
      puts "| #{ascii_part}"
    end

    if data.length > max_bytes
      puts "... (showing first #{max_bytes} of #{data.length} bytes)"
    end
    puts
  end
end

# Command line interface
def main
  options = {
    hex_dump: false,
    disasm: false,
    start_offset: 0,
    length: nil,
    base_address: 0x8000,
    max_hex: 256
  }

  OptionParser.new do |opts|
    opts.banner = "Usage: #{$0} [options] <nes_file>"
    opts.separator ""
    opts.separator "Options:"

    opts.on("-d", "--disasm", "Disassemble PRG ROM") do
      options[:disasm] = true
    end

    opts.on("-x", "--hex", "Show hex dump of ROM sections") do
      options[:hex_dump] = true
    end

    opts.on("-s", "--start OFFSET", Integer, "Start offset for disassembly (default: 0)") do |s|
      options[:start_offset] = s
    end

    opts.on("-l", "--length LENGTH", Integer, "Length of disassembly in bytes") do |l|
      options[:length] = l
    end

    opts.on("-b", "--base ADDRESS", Integer, "Base address for disassembly (default: 0x8000)") do |b|
      options[:base_address] = b
    end

    opts.on("-m", "--max-hex BYTES", Integer, "Maximum bytes to show in hex dump (default: 256)") do |m|
      options[:max_hex] = m
    end

    opts.on("-h", "--help", "Show this help message") do
      puts opts
      exit 0
    end
  end.parse!

  if ARGV.empty?
    puts "Error: Please specify a NES ROM file"
    puts "Use -h for help"
    exit 1
  end

  filename = ARGV[0]
  unless File.exist?(filename)
    puts "Error: File '#{filename}' not found"
    exit 1
  end

  begin
    parser = NESRomParser.new(filename)
    
    unless parser.parse
      puts "Error: Failed to parse NES ROM file"
      exit 1
    end

    # Always show header
    parser.print_header

    # Show hex dumps if requested
    if options[:hex_dump]
      if parser.trainer
        parser.hex_dump(parser.trainer, "Trainer", 0x7000, options[:max_hex])
      end
      
      if parser.prg_rom
        parser.hex_dump(parser.prg_rom, "PRG ROM", 0x8000, options[:max_hex])
      end
      
      if parser.chr_rom
        parser.hex_dump(parser.chr_rom, "CHR ROM", 0x0000, options[:max_hex])
      end
    end

    # Disassemble if requested
    if options[:disasm] && parser.prg_rom
      parser.disassemble_prg_rom(
        options[:start_offset],
        options[:length],
        options[:base_address]
      )
    end

    # If no specific options, show full ROM disassembly
    unless options[:hex_dump] || options[:disasm]
      puts "Full ROM Disassembly (use -d for custom range, -x for hex dump, -h for help)"
      puts
      
      # Disassemble entire PRG ROM
      if parser.prg_rom
        parser.disassemble_prg_rom(0, nil, 0x8000)
      end
    end

  rescue => e
    puts "Error: #{e.message}"
    exit 1
  end
end

# Run the program
if __FILE__ == $0
  main
end