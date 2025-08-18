#!/usr/bin/env ruby
# Build continuous tone test NSF
# Creates an NSF file that plays continuous tones on all APU channels

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

# 6502 assembly code for NSF
nsf_code = [
  # init routine at $8000
  0xA9, 0x0F,        # LDA #$0F
  0x8D, 0x15, 0x40,  # STA $4015    ; Enable all channels
  
  # Pulse 1 setup (440Hz)
  0xA9, 0xBF,        # LDA #$BF     ; Duty=10, hold, vol=15
  0x8D, 0x00, 0x40,  # STA $4000
  0xA9, 0x00,        # LDA #$00     ; No sweep
  0x8D, 0x01, 0x40,  # STA $4001
  0xA9, 0xFD,        # LDA #$FD     ; Period low
  0x8D, 0x02, 0x40,  # STA $4002
  0xA9, 0x00,        # LDA #$00     ; Period high
  0x8D, 0x03, 0x40,  # STA $4003
  
  # Pulse 2 setup (523Hz - C5)
  0xA9, 0xBF,        # LDA #$BF     ; Duty=10, hold, vol=15
  0x8D, 0x04, 0x40,  # STA $4004
  0xA9, 0x00,        # LDA #$00     ; No sweep
  0x8D, 0x05, 0x40,  # STA $4005
  0xA9, 0x55,        # LDA #$55     ; Period low
  0x8D, 0x06, 0x40,  # STA $4006
  0xA9, 0x00,        # LDA #$00     ; Period high
  0x8D, 0x07, 0x40,  # STA $4007
  
  0x60,              # RTS ($801F)
  
  # play routine at $8020
  0xA9, 0x0F,        # LDA #$0F
  0x8D, 0x15, 0x40,  # STA $4015    ; Keep channels enabled
  0x60,              # RTS
].pack("C*")

# Pad the code to fill the NSF data area (32KB)
nsf_data = nsf_code.ljust(32768, "\x00")

# Combine header and data
nsf_file = nsf_header + nsf_data

# Write the NSF file
File.open('continuous_tone.nsf', 'wb') do |f|
  f.write(nsf_file)
end

puts "Created continuous_tone.nsf (#{nsf_file.bytesize} bytes)"
puts "This NSF plays continuous tones:"
puts "  - Pulse 1: 440Hz (A4)"
puts "  - Pulse 2: 523Hz (C5)"
puts "Use this to test if the audio quality degrades over time"