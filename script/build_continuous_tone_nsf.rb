#!/usr/bin/env ruby
# Build continuous tone test NSF from assembly source
# Creates an NSF file that plays continuous tones on all APU channels

require 'json'

filename = "continuous_tone_single"

# 6502 instruction encoding
def assemble_line(line)
  line = line.strip.gsub(/;.*/, '').strip  # Remove comments
  return [] if line.empty? || line.start_with?(';') || line.start_with?('.') || line.end_with?(':')
  
  case line
  when /^LDA #\$([0-9A-F]+)$/i
    [0xA9, $1.to_i(16)]
  when /^STA \$([0-9A-F]+)$/i
    addr = $1.to_i(16)
    [0x8D, addr & 0xFF, (addr >> 8) & 0xFF]
  when /^RTS$/i
    [0x60]
  else
    puts "Warning: Unknown instruction: #{line}"
    []
  end
end

# Read and assemble the .asm file
def assemble_file(filename)
  code = []
  current_org = 0x8000
  
  File.readlines(filename).each do |line|
    line = line.strip
    
    if line =~ /^\.org \$([0-9A-F]+)$/i
      target_org = $1.to_i(16)
      # Pad to reach target address
      while (current_org + code.length) < target_org
        code << 0x00
      end
      current_org = target_org
    else
      assembled = assemble_line(line)
      code.concat(assembled)
    end
  end
  
  code.pack("C*")
end

# NSF header structure
nsf_header = [
  0x4E, 0x45, 0x53, 0x4D, 0x1A,  # "NESM" + 0x1A
  0x01,                            # Version
  0x01,                            # Total songs
  0x01,                            # Starting song
  0x00, 0x80,                      # Load address ($8000)
  0x00, 0x80,                      # Init address ($8000)
  0x20, 0x80,                      # Play address ($8020)
].pack("C*")

# Song name (32 bytes)
nsf_header += "Continuous Tone Test".ljust(32, "\x00")
# Artist (32 bytes)
nsf_header += "APU Test".ljust(32, "\x00")
# Copyright (32 bytes)
nsf_header += "Test NSF".ljust(32, "\x00")
# NTSC speed
nsf_header += [0x6A, 0x66].pack("C*")
# Bankswitch (8 bytes)
nsf_header += "\x00" * 8
# PAL speed
nsf_header += [0x20, 0x4E].pack("C*")
# PAL/NTSC bits, Extra sound chip
nsf_header += [0x00, 0x00].pack("C*")
# Reserved (4 bytes)
nsf_header += "\x00" * 4

# Assemble the code from .asm file
asm_file = File.join(File.dirname(__FILE__), "#{filename}.asm")
nsf_code = assemble_file(asm_file)

# Pad the code to fill the NSF data area (32KB)
nsf_data = nsf_code.ljust(32768, "\x00")

# Combine header and data
nsf_file = nsf_header + nsf_data

# Write the NSF file
File.open("#{filename}.nsf", 'wb') do |f|
  f.write(nsf_file)
end

puts "Created #{filename}.nsf (#{nsf_file.bytesize} bytes)"
