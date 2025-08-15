#!/usr/bin/env python3

import struct

# NSF header structure
def create_minimal_nsf():
    # NSF Header (128 bytes)
    header = bytearray(128)
    
    # Signature: "NESM" + 0x1A
    header[0:5] = b'NESM\x1A'
    
    # Version
    header[5] = 1
    
    # Total songs
    header[6] = 1
    
    # Starting song (1-based)
    header[7] = 1
    
    # Load address (little endian) - $8000
    header[8:10] = struct.pack('<H', 0x8000)
    
    # Init address (little endian) - $8000
    header[10:12] = struct.pack('<H', 0x8000)
    
    # Play address (little endian) - $8004
    header[12:14] = struct.pack('<H', 0x8004)
    
    # Song name
    header[14:46] = b'Test NSF - NOP+RTS\x00' + b'\x00' * (32 - len(b'Test NSF - NOP+RTS\x00'))
    
    # Artist
    header[46:78] = b'Claude Code\x00' + b'\x00' * (32 - len(b'Claude Code\x00'))
    
    # Copyright
    header[78:110] = b'2025\x00' + b'\x00' * (32 - len(b'2025\x00'))
    
    # NTSC speed (16666 = 1/60 sec in microseconds)
    header[110:112] = struct.pack('<H', 16666)
    
    # Bankswitch (8 bytes of 0x00 = no bankswitch)
    header[112:120] = b'\x00' * 8
    
    # PAL speed (not used)
    header[120:122] = struct.pack('<H', 0)
    
    # PAL/NTSC flags (0 = NTSC)
    header[122] = 0
    
    # Extra sound (0 = standard 2A03)
    header[123] = 0
    
    # Reserved (4 bytes)
    header[124:128] = b'\x00' * 4
    
    # Code section - minimal test
    code = bytearray()
    
    # INIT routine at $8000:
    code.extend([
        0xEA,  # $8000: NOP
        0xEA,  # $8001: NOP  
        0xEA,  # $8002: NOP
        0x60   # $8003: RTS
    ])
    
    # PLAY routine at $8004:
    code.extend([
        0xEA,  # $8004: NOP
        0xEA,  # $8005: NOP
        0x60   # $8006: RTS
    ])
    
    return header + code

# Create the NSF file
nsf_data = create_minimal_nsf()

# Write to file
with open('/home/kishima/dev/esp32_nes_apu_mruby/spiffs_data/nsf/minimal_test.nsf', 'wb') as f:
    f.write(nsf_data)

print(f"Created minimal NSF file: {len(nsf_data)} bytes")
print("Header: 128 bytes")
print(f"Code: {len(nsf_data) - 128} bytes")
print()
print("INIT routine ($8000):")
print("  $8000: EA    NOP")
print("  $8001: EA    NOP") 
print("  $8002: EA    NOP")
print("  $8003: 60    RTS")
print()
print("PLAY routine ($8004):")
print("  $8004: EA    NOP")
print("  $8005: EA    NOP")
print("  $8006: 60    RTS")