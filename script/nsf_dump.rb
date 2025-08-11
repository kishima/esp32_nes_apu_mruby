#!/usr/bin/env ruby
# NSF (NES Sound Format) File Dumper and 6502 Disassembler
# Parses NSF music files and displays machine code with assembly disassembly

require 'optparse'

# 6502 Disassembler for NSF files (simplified version)
class NSF6502Disassembler
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

  # Key 6502 instructions for NSF analysis
  INSTRUCTIONS = {
    0x69 => { mnemonic: "ADC", mode: :immediate, desc: "Add immediate value to accumulator with carry" },
    0x6D => { mnemonic: "ADC", mode: :absolute, desc: "Add absolute value to accumulator with carry" },
    0x29 => { mnemonic: "AND", mode: :immediate, desc: "AND immediate value with accumulator" },
    0x2D => { mnemonic: "AND", mode: :absolute, desc: "AND absolute value with accumulator" },
    0x0A => { mnemonic: "ASL", mode: :accumulator, desc: "Shift accumulator left one bit" },
    0x90 => { mnemonic: "BCC", mode: :relative, desc: "Branch if carry clear" },
    0xB0 => { mnemonic: "BCS", mode: :relative, desc: "Branch if carry set" },
    0xF0 => { mnemonic: "BEQ", mode: :relative, desc: "Branch if equal (zero flag set)" },
    0x30 => { mnemonic: "BMI", mode: :relative, desc: "Branch if minus (negative flag set)" },
    0xD0 => { mnemonic: "BNE", mode: :relative, desc: "Branch if not equal (zero flag clear)" },
    0x10 => { mnemonic: "BPL", mode: :relative, desc: "Branch if plus (negative flag clear)" },
    0x00 => { mnemonic: "BRK", mode: :implicit, desc: "Force interrupt (software break)" },
    0x18 => { mnemonic: "CLC", mode: :implicit, desc: "Clear carry flag" },
    0x58 => { mnemonic: "CLI", mode: :implicit, desc: "Clear interrupt disable flag" },
    0xC9 => { mnemonic: "CMP", mode: :immediate, desc: "Compare accumulator with immediate value" },
    0xCD => { mnemonic: "CMP", mode: :absolute, desc: "Compare accumulator with absolute value" },
    0xE0 => { mnemonic: "CPX", mode: :immediate, desc: "Compare X register with immediate value" },
    0xC0 => { mnemonic: "CPY", mode: :immediate, desc: "Compare Y register with immediate value" },
    0xCA => { mnemonic: "DEX", mode: :implicit, desc: "Decrement X register" },
    0x88 => { mnemonic: "DEY", mode: :implicit, desc: "Decrement Y register" },
    0x49 => { mnemonic: "EOR", mode: :immediate, desc: "XOR accumulator with immediate value" },
    0xE8 => { mnemonic: "INX", mode: :implicit, desc: "Increment X register" },
    0xC8 => { mnemonic: "INY", mode: :implicit, desc: "Increment Y register" },
    0x4C => { mnemonic: "JMP", mode: :absolute, desc: "Jump to absolute address" },
    0x20 => { mnemonic: "JSR", mode: :absolute, desc: "Jump to subroutine" },
    0xA9 => { mnemonic: "LDA", mode: :immediate, desc: "Load accumulator with immediate value" },
    0xAD => { mnemonic: "LDA", mode: :absolute, desc: "Load accumulator with absolute value" },
    0xA2 => { mnemonic: "LDX", mode: :immediate, desc: "Load X register with immediate value" },
    0xAE => { mnemonic: "LDX", mode: :absolute, desc: "Load X register with absolute value" },
    0xA0 => { mnemonic: "LDY", mode: :immediate, desc: "Load Y register with immediate value" },
    0xAC => { mnemonic: "LDY", mode: :absolute, desc: "Load Y register with absolute value" },
    0x4A => { mnemonic: "LSR", mode: :accumulator, desc: "Shift accumulator right one bit" },
    0xEA => { mnemonic: "NOP", mode: :implicit, desc: "No operation" },
    0x09 => { mnemonic: "ORA", mode: :immediate, desc: "OR accumulator with immediate value" },
    0x0D => { mnemonic: "ORA", mode: :absolute, desc: "OR accumulator with absolute value" },
    0x48 => { mnemonic: "PHA", mode: :implicit, desc: "Push accumulator onto stack" },
    0x68 => { mnemonic: "PLA", mode: :implicit, desc: "Pull accumulator from stack" },
    0x40 => { mnemonic: "RTI", mode: :implicit, desc: "Return from interrupt" },
    0x60 => { mnemonic: "RTS", mode: :implicit, desc: "Return from subroutine" },
    0xE9 => { mnemonic: "SBC", mode: :immediate, desc: "Subtract immediate value from accumulator with borrow" },
    0xED => { mnemonic: "SBC", mode: :absolute, desc: "Subtract absolute value from accumulator with borrow" },
    0x38 => { mnemonic: "SEC", mode: :implicit, desc: "Set carry flag" },
    0x78 => { mnemonic: "SEI", mode: :implicit, desc: "Set interrupt disable flag" },
    0x8D => { mnemonic: "STA", mode: :absolute, desc: "Store accumulator to absolute address" },
    0x8E => { mnemonic: "STX", mode: :absolute, desc: "Store X register to absolute address" },
    0x8C => { mnemonic: "STY", mode: :absolute, desc: "Store Y register to absolute address" },
    0xAA => { mnemonic: "TAX", mode: :implicit, desc: "Transfer accumulator to X register" },
    0xA8 => { mnemonic: "TAY", mode: :implicit, desc: "Transfer accumulator to Y register" },
    0x8A => { mnemonic: "TXA", mode: :implicit, desc: "Transfer X register to accumulator" },
    0x98 => { mnemonic: "TYA", mode: :implicit, desc: "Transfer Y register to accumulator" }
  }

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
    opcode = opcode.ord if opcode.is_a?(String)
    instruction = INSTRUCTIONS[opcode]
    
    unless instruction
      return {
        bytes: [opcode],
        hex: "%02X" % opcode,
        mnemonic: "???",
        operand: "",
        desc: "Illegal or undocumented opcode",
        size: 1
      }
    end

    size = ADDRESSING_MODES[instruction[:mode]]
    bytes = data[0, size] || [opcode]
    
    bytes = bytes.bytes if bytes.is_a?(String)
    
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
      desc: instruction[:desc] || "Unknown instruction",
      size: size
    }
  end
end

class NSFParser
  attr_reader :header, :data, :code_data
  
  NSF_MAGIC = "NESM\x1A".freeze

  def initialize(filename)
    @filename = filename
    @data = nil
    @header = {}
    @code_data = nil
  end

  def parse
    File.open(@filename, 'rb') do |file|
      @data = file.read
    end

    return false unless parse_header
    parse_code_data
    true
  end

  private

  def parse_header
    return false if @data.length < 128  # NSF header is 128 bytes
    return false unless @data[0, 5] == NSF_MAGIC

    bytes = @data.bytes
    
    @header = {
      magic: @data[0, 5],
      version: bytes[5],
      total_songs: bytes[6],
      starting_song: bytes[7],
      load_address: bytes[8] | (bytes[9] << 8),
      init_address: bytes[10] | (bytes[11] << 8),
      play_address: bytes[12] | (bytes[13] << 8),
      song_name: @data[14, 32].strip.delete("\x00"),
      artist: @data[46, 32].strip.delete("\x00"),
      copyright: @data[78, 32].strip.delete("\x00"),
      ntsc_speed: bytes[110] | (bytes[111] << 8),
      bankswitch_init: @data[112, 8],
      pal_speed: bytes[120] | (bytes[121] << 8),
      region: bytes[122],  # 0=NTSC, 1=PAL, 2=Dual
      extra_sound: bytes[123],
      expansion: @data[124, 4]
    }

    true
  end

  def parse_code_data
    # Code data starts after 128-byte header
    @code_data = @data[128..-1] if @data.length > 128
  end

  public

  def print_header
    puts "=" * 70
    puts "NSF Header Information"
    puts "=" * 70
    puts "File: #{@filename}"
    puts "Magic: #{@header[:magic].unpack('H*')[0].upcase}"
    puts "Version: #{@header[:version]}"
    puts "Total Songs: #{@header[:total_songs]}"
    puts "Starting Song: #{@header[:starting_song]} (1-based)"
    puts "Load Address: $%04X" % @header[:load_address]
    puts "Init Address: $%04X" % @header[:init_address]  
    puts "Play Address: $%04X" % @header[:play_address]
    puts "Song Name: '#{@header[:song_name]}'"
    puts "Artist: '#{@header[:artist]}'"
    puts "Copyright: '#{@header[:copyright]}'"
    puts "NTSC Speed: #{@header[:ntsc_speed]} (1/1000000 sec)"
    puts "PAL Speed: #{@header[:pal_speed]} (1/1000000 sec)"
    
    region_str = case @header[:region]
                when 0 then "NTSC"
                when 1 then "PAL"
                when 2 then "Dual (NTSC/PAL)"
                else "Unknown (#{@header[:region]})"
                end
    puts "Region: #{region_str}"
    
    # Expansion audio chips
    if @header[:extra_sound] > 0
      expansions = []
      expansions << "VRC6" if (@header[:extra_sound] & 0x01) != 0
      expansions << "VRC7" if (@header[:extra_sound] & 0x02) != 0
      expansions << "FDS" if (@header[:extra_sound] & 0x04) != 0
      expansions << "MMC5" if (@header[:extra_sound] & 0x08) != 0
      expansions << "Namco 163" if (@header[:extra_sound] & 0x10) != 0
      expansions << "Sunsoft 5B" if (@header[:extra_sound] & 0x20) != 0
      puts "Extra Sound: #{expansions.join(', ')}"
    else
      puts "Extra Sound: None (Standard 2A03 APU only)"
    end

    # Bankswitch info
    bankswitch = @header[:bankswitch_init].unpack('C*')
    if bankswitch.any? { |b| b != 0 }
      puts "Bankswitch Init: #{bankswitch.map { |b| '%02X' % b }.join(' ')}"
    else
      puts "Bankswitch Init: None (Linear addressing)"
    end
    
    if @code_data
      puts "Code Data Size: #{@code_data.length} bytes"
    end
    puts
  end

  def print_nsf_memory_map
    puts "=" * 70
    puts "NSF Memory Map and APU Register Reference"
    puts "=" * 70
    puts "Load Address: $%04X - Code/data loaded here" % @header[:load_address]
    puts "Init Address: $%04X - Called to initialize song" % @header[:init_address]  
    puts "Play Address: $%04X - Called 50/60 times per second" % @header[:play_address]
    puts
    puts "APU Registers (2A03 Sound Channels):"
    puts "$4000-$4003  Pulse 1 (Square wave with sweep)"
    puts "$4004-$4007  Pulse 2 (Square wave with sweep)" 
    puts "$4008-$400B  Triangle (Linear counter control)"
    puts "$400C-$400F  Noise (Pseudo-random noise)"
    puts "$4010-$4013  DMC (Delta modulation channel)"
    puts "$4015        Channel enable/disable and length counter status"
    puts "$4017        Frame counter (4 or 5-step mode)"
    puts
    puts "Memory Regions:"
    puts "$0000-$07FF  RAM (2KB)"
    puts "$0800-$1FFF  RAM mirrors"
    puts "$2000-$3FFF  PPU registers (not used in NSF)"
    puts "$4000-$4017  APU registers"
    puts "$4018-$401F  APU test registers"
    puts "$4020-$5FFF  Expansion area"
    puts "$6000-$7FFF  Battery-backed RAM"
    puts "$8000-$FFFF  Code/data area"
    puts
  end

  def disassemble_code(start_offset = 0, length = nil, base_address = nil)
    return unless @code_data

    disassembler = NSF6502Disassembler.new
    base_address ||= @header[:load_address]
    
    # Default to entire code data if no length specified
    length ||= @code_data.length - start_offset
    length = [@code_data.length - start_offset, length].min

    puts "=" * 100
    puts "NSF Code Disassembly (Offset: $%04X, Length: %d bytes, Base: $%04X)" % [start_offset, length, base_address]
    puts "=" * 100
    puts "Address  | Hex Code     | Assembly             ; Description"
    puts "-" * 100

    offset = start_offset
    address = base_address

    while offset < start_offset + length && offset < @code_data.length
      data = @code_data[offset..-1]
      break if data.empty?

      instruction = disassembler.disassemble_instruction(data, address)
      
      # Format the output with description
      hex_padding = " " * (12 - instruction[:hex].length)
      operand_part = instruction[:operand].empty? ? "" : " #{instruction[:operand]}"
      assembly_part = "#{instruction[:mnemonic]}#{operand_part}"
      
      # Highlight APU register access
      comment = instruction[:desc]
      if assembly_part =~ /\$40[0-1][0-9A-F]/
        comment += " [APU Register]"
      end
      
      puts "$%04X    | %-12s | %-20s ; %s" % [
        address,
        instruction[:hex],
        assembly_part,
        comment
      ]

      offset += instruction[:size]
      address += instruction[:size]
    end
    puts
  end

  def disassemble_init_routine
    return unless @code_data
    
    init_offset = @header[:init_address] - @header[:load_address]
    if init_offset >= 0 && init_offset < @code_data.length
      puts "=" * 70
      puts "INIT Routine Disassembly (Entry Point: $%04X)" % @header[:init_address]
      puts "=" * 70
      disassemble_code(init_offset, 64, @header[:init_address])
    else
      puts "INIT address $%04X is outside loaded code range" % @header[:init_address]
    end
  end

  def disassemble_play_routine  
    return unless @code_data
    
    play_offset = @header[:play_address] - @header[:load_address]
    if play_offset >= 0 && play_offset < @code_data.length
      puts "=" * 70
      puts "PLAY Routine Disassembly (Entry Point: $%04X)" % @header[:play_address]
      puts "=" * 70
      disassemble_code(play_offset, 64, @header[:play_address])
    else
      puts "PLAY address $%04X is outside loaded code range" % @header[:play_address]
    end
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

  def analyze_apu_usage
    return unless @code_data
    
    puts "=" * 70
    puts "APU Register Usage Analysis"
    puts "=" * 70
    
    apu_writes = Hash.new(0)
    disassembler = NSF6502Disassembler.new
    
    offset = 0
    address = @header[:load_address]

    while offset < @code_data.length
      data = @code_data[offset..-1]
      break if data.empty?

      instruction = disassembler.disassemble_instruction(data, address)
      
      # Look for STA instructions to APU registers
      if instruction[:mnemonic] == "STA" && instruction[:operand] =~ /\$40([0-1][0-9A-F])/
        reg_addr = $1.to_i(16) + 0x4000
        apu_writes[reg_addr] += 1
      end

      offset += instruction[:size]
      address += instruction[:size]
    end
    
    if apu_writes.any?
      apu_names = {
        0x4000 => "SQ1_VOL  (Pulse 1 Volume/Duty)",
        0x4001 => "SQ1_SWEEP (Pulse 1 Sweep)",
        0x4002 => "SQ1_LO   (Pulse 1 Period Low)",
        0x4003 => "SQ1_HI   (Pulse 1 Period High)",
        0x4004 => "SQ2_VOL  (Pulse 2 Volume/Duty)",
        0x4005 => "SQ2_SWEEP (Pulse 2 Sweep)",
        0x4006 => "SQ2_LO   (Pulse 2 Period Low)",
        0x4007 => "SQ2_HI   (Pulse 2 Period High)",
        0x4008 => "TRI_LINEAR (Triangle Linear Counter)",
        0x400A => "TRI_LO   (Triangle Period Low)",
        0x400B => "TRI_HI   (Triangle Period High)",
        0x400C => "NOISE_VOL (Noise Volume)",
        0x400E => "NOISE_LO (Noise Period)",
        0x400F => "NOISE_HI (Noise Length)",
        0x4010 => "DMC_FREQ (DMC Frequency)",
        0x4011 => "DMC_RAW  (DMC Output)",
        0x4012 => "DMC_START (DMC Sample Address)",
        0x4013 => "DMC_LEN  (DMC Sample Length)",
        0x4015 => "SND_CHN  (Channel Enable)",
        0x4017 => "JOY2     (Frame Counter)"
      }
      
      apu_writes.keys.sort.each do |addr|
        name = apu_names[addr] || "UNKNOWN"
        count = apu_writes[addr]
        puts "$%04X  %-25s  (%d writes)" % [addr, name, count]
      end
    else
      puts "No direct APU register writes found in static analysis"
    end
    puts
  end
end

# Command line interface
def main
  options = {
    hex_dump: false,
    disasm: false,
    init_routine: false,
    play_routine: false,
    memory_map: false,
    apu_analysis: false,
    start_offset: 0,
    length: nil,
    max_hex: 256
  }

  OptionParser.new do |opts|
    opts.banner = "Usage: #{$0} [options] <nsf_file>"
    opts.separator ""
    opts.separator "NSF (NES Sound Format) file analyzer and disassembler"
    opts.separator ""
    opts.separator "Options:"

    opts.on("-d", "--disasm", "Disassemble entire NSF code") do
      options[:disasm] = true
    end

    opts.on("-i", "--init", "Disassemble INIT routine") do
      options[:init_routine] = true
    end

    opts.on("-p", "--play", "Disassemble PLAY routine") do
      options[:play_routine] = true
    end

    opts.on("-m", "--memory-map", "Show NSF memory map and APU registers") do
      options[:memory_map] = true
    end

    opts.on("-a", "--apu-analysis", "Analyze APU register usage") do
      options[:apu_analysis] = true
    end

    opts.on("-x", "--hex", "Show hex dump of code data") do
      options[:hex_dump] = true
    end

    opts.on("-s", "--start OFFSET", Integer, "Start offset for disassembly") do |s|
      options[:start_offset] = s
    end

    opts.on("-l", "--length LENGTH", Integer, "Length of disassembly in bytes") do |l|
      options[:length] = l
    end

    opts.on("--max-hex BYTES", Integer, "Maximum bytes in hex dump (default: 256)") do |m|
      options[:max_hex] = m
    end

    opts.on("-h", "--help", "Show this help message") do
      puts opts
      exit 0
    end
  end.parse!

  if ARGV.empty?
    puts "Error: Please specify an NSF file"
    puts "Use -h for help"
    exit 1
  end

  filename = ARGV[0]
  unless File.exist?(filename)
    puts "Error: File '#{filename}' not found"
    exit 1
  end

  begin
    parser = NSFParser.new(filename)
    
    unless parser.parse
      puts "Error: Failed to parse NSF file"
      exit 1
    end

    # Always show header
    parser.print_header

    # Show memory map if requested
    if options[:memory_map]
      parser.print_nsf_memory_map
    end

    # Show hex dump if requested  
    if options[:hex_dump] && parser.code_data
      parser.hex_dump(parser.code_data, "NSF Code Data", parser.header[:load_address], options[:max_hex])
    end

    # APU analysis
    if options[:apu_analysis]
      parser.analyze_apu_usage
    end

    # Disassemble routines
    if options[:init_routine]
      parser.disassemble_init_routine
    end

    if options[:play_routine]
      parser.disassemble_play_routine
    end

    # Full disassembly
    if options[:disasm] && parser.code_data
      parser.disassemble_code(
        options[:start_offset],
        options[:length],
        parser.header[:load_address] + options[:start_offset]
      )
    end

    # Default behavior - show overview
    unless options.values.any?
      puts "Quick Analysis (use -h for more options):"
      puts
      parser.analyze_apu_usage
      parser.disassemble_init_routine
      parser.disassemble_play_routine
    end

  rescue => e
    puts "Error: #{e.message}"
    puts e.backtrace if $DEBUG
    exit 1
  end
end

# Run the program
if __FILE__ == $0
  main
end