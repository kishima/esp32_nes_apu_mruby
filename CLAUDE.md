# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-based NES (Nintendo Entertainment System) APU (Audio Processing Unit) emulator built on the ESP-IDF v5.4 framework. It's derived from Peter Barrett's esp_8_bit project but focused specifically on audio emulation. The project uses the Nofrendo NES emulator core for cycle-accurate emulation.

## Build System

**Primary Build Method:**
```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
./.devcontainer/run_devcontainer.sh idf.py monitor
```

**Docker Build Method:**
```bash
# Build container first
cd .devcontainer
./build.sh

# Then use container to build project
# (specific container run commands depend on your setup)
```

**Configuration:**
- Uses ESP-IDF component system with main application and apu_emu component
- SPIFFS filesystem for ROM storage (256KB partition)
- Bluetooth functionality has been completely removed (APU-focused build)
- Video output is stubbed (focus on audio emulation)

## Architecture Overview

### Core Components Structure
```
main.cpp (ESP32 Application Entry)
├── Filesystem mounting (SPIFFS)
├── Emulator initialization (Nofrendo core)
├── HID stub initialization (input disabled)
└── Dual-core execution management

components/apu_emu/
├── src/emu.h - Generic emulator interface
├── src/emu_nofrendo.cpp - NES-specific implementation  
├── src/gui.cpp - User interface management
├── src/hid_stub.cpp - Input stubs (Bluetooth removed)
└── src/nofrendo/ - Complete NES emulation core
    ├── nes_apu.c/h - APU (Audio) emulation
    ├── nes_ppu.c/h - PPU (Graphics) emulation
    ├── nes6502.c/h - CPU (6502) emulation
    ├── nes.c/h - Main NES system controller
    └── [various mapper implementations]
```

### APU (Audio Processing Unit) Architecture
The NES APU emulates 5 audio channels:
- **2 Pulse Wave Channels** (0x4000-0x4007): Rectangle waves with duty cycle control
- **1 Triangle Wave Channel** (0x4008-0x400B): Linear triangle wave
- **1 Noise Channel** (0x400C-0x400F): Pseudo-random noise generator  
- **1 DMC Channel** (0x4010-0x4013): Delta Modulation Channel for samples

**Audio Data Flow:**
```
NES CPU writes → APU registers (0x4000-0x4017) → Channel state updates → 
Sample generation → osd_setsound() callback → EmuNofrendo::audio_buffer() → 
audio_write_16() stub (future ESP32 I2S/DAC output)
```

### Dual-Core Execution Model
- **Core 0**: Emulator task (`emu_task`) - game logic, APU, graphics processing
- **Core 1**: Main application loop - HID processing (stubbed), performance monitoring
- Toggle with `#define SINGLE_CORE` for single-core operation

### Memory Management
- ROM files loaded via memory mapping for efficiency
- Direct video buffer rendering (though output is stubbed)
- Optimized for ESP32's limited RAM constraints

## Key Files for APU Development

**APU Core Implementation:**
- `src/nofrendo/nes_apu.c/h` - Main APU emulation logic
- `src/nofrendo/osd.c` - Audio output system interface
- `src/emu_nofrendo.cpp` - Audio buffer management and format conversion

**Audio Output Integration Points:**
- `main.cpp:audio_write_16()` - Stub for ESP32 audio output (I2S/DAC)
- `EmuNofrendo::audio_buffer()` - Converts 8-bit unsigned to 16-bit signed samples
- Audio sample rate: ~15.7kHz (NTSC) / ~15.6kHz (PAL)

## Configuration Files

- `sdkconfig.defaults` - ESP-IDF project configuration (Bluetooth disabled)
- `partitions.csv` - Flash partition layout
- `CMakeLists.txt` - Root project configuration
- `components/apu_emu/CMakeLists.txt` - Component build configuration

## Current State

- ✅ NES emulation core fully functional
- ✅ APU emulation working (5 channels implemented)
- ✅ ROM loading from SPIFFS
- ❌ Audio output (stubbed - needs I2S/DAC implementation)
- ❌ Video output (stubbed)
- ❌ Input system (stubbed - Bluetooth HID removed)

## Important Technical Notes

- Project requires ESP-IDF v5.4+ (not Arduino framework)
- All Bluetooth/HID server components have been removed for APU focus
- Video output functions are stubs - only composite video structures remain
- Audio processing is cycle-accurate following NES timing specifications
- ROM files should be placed in SPIFFS filesystem under `/nofrendo/` directory