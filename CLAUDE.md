# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-based NES emulator project derived from Peter Barrett's esp_8_bit, converted from Arduino IDE to ESP-IDF build system. Despite the name suggesting APU focus, this is currently a **complete NES emulator** with video, audio, and input systems. The project includes Bluetooth HID support that has been partially removed/stubbed for APU-focused builds.

**Key Difference from Original**: Converted from Arduino framework to ESP-IDF v5.4+ component architecture for better hardware control and memory management.

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

# Then use container for development
```

**Configuration:**
- Requires ESP-IDF v5.4+ (not Arduino)
- Custom partition table with 256KB SPIFFS for ROM storage
- Bluetooth functionality is stubbed/disabled for APU builds
- Uses dual-core architecture: Core 0 for emulation, Core 1 for system tasks

## Architecture Overview

### Core Components Structure
```
main.cpp (ESP32 Application Entry)
├── Filesystem mounting (SPIFFS) 
├── Emulator initialization (Nofrendo/Atari800/SMSPlus)
├── HID stub initialization (Bluetooth removed)
└── Dual-core execution with video/audio output

components/apu_emu/
├── src/emu.h - Generic emulator interface
├── src/emu.cpp - Base emulator framework & file system
├── src/emu_nofrendo.cpp - NES-specific implementation  
├── src/gui.cpp - User interface and file browser
├── src/hid.cpp - Input stubs (Bluetooth HID removed)
├── src/video_out.h - ESP32 video/audio hardware abstraction
└── src/nofrendo/ - Complete NES emulation core
    ├── nes_apu.c/h - APU (5-channel audio) emulation
    ├── nes_ppu.c/h - PPU (graphics) emulation  
    ├── nes6502.c/h - CPU (6502) emulation
    ├── nes.c/h - Main NES system controller
    └── [80+ memory mapper implementations]
```

### Software Architecture Layers

The emulator follows a clean layered architecture with three main C++ components:

```
┌─────────────────────────────────────────┐
│        GUI Layer (gui.cpp)              │
│  File Browser │ Input Handler │ Overlay │
│  - ROM file selection and browsing      │
│  - USB HID keyboard input processing    │
│  - On-screen display and UI rendering   │
│  - Menu navigation and user interaction │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────┴───────────────────────┐
│   Emulation Layer (emu_nofrendo.cpp)   │
│    NES Core │ Audio/Video │ Input Map   │
│  - NES-specific emulation using Nofrendo│
│  - NTSC/PAL palette and timing control  │
│  - NES controller input mapping         │
│  - APU audio processing and buffering   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────┴───────────────────────┐
│        Base Layer (emu.cpp)             │
│  File System │ Memory Mgmt │ Framework  │
│  - CrapFS: Custom flash filesystem      │
│  - ROM memory mapping for large files   │
│  - Abstract Emu base class framework    │
│  - Cross-platform file loading utilities│
└─────────────────────────────────────────┘
```

**Layer Interactions:**
- **GUI → Emulation**: Calls `emu->insert()` for ROM loading, `emu->update()` for frame updates
- **Emulation → Base**: Uses file loading, memory mapping, audio/video infrastructure  
- **GUI ← Emulation**: Retrieves video buffers via `video_buffer()`, audio via `audio_buffer()`

### NES APU (Audio Processing Unit) Architecture
**Hardware-accurate 5-channel audio synthesizer:**
- **Pulse Channel 1 & 2** (0x4000-0x4007): Square waves with duty cycle, envelope, sweep
- **Triangle Channel** (0x4008-0x400B): Linear triangle wave for bass
- **Noise Channel** (0x400C-0x400F): LFSR-based noise generation
- **DMC Channel** (0x4010-0x4013): Delta modulation for samples

**Audio Processing Pipeline:**
```
NES CPU register writes → APU channel updates → Sample generation (15.7kHz) → 
audio_write_16() → PWM output (GPIO18) → RC filter → Audio out
```

### ESP32 Hardware Integration
- **Video Output**: I2S DMA → DAC (GPIO25) → Composite video
- **Audio Output**: LEDC PWM (GPIO18) → RC filter required  
- **Input**: Bluetooth HID (stubbed) or IR (GPIO0)
- **Storage**: SPIFFS partition for ROM files

### Memory Management & ROM Loading
**CrapFS System**: Custom file system overlay on ESP32 flash partition for large ROM caching
**Memory Mapping**: Direct SPI flash mapping for efficient ROM access without RAM copying
**PSRAM Usage**: Video frame buffers and emulation state stored in PSRAM when available

### Core C++ Files Detailed Responsibilities

#### **emu.cpp** - Foundation & File System Layer
**Primary Role**: Provides base emulator framework and ROM file management infrastructure

**Key Components:**
- **CrapFS Class**: Custom flash filesystem for storing ROM files in ESP32 partition
  - `mmap()`: Memory-maps files from flash partition for zero-copy access
  - `create()`: Creates new files in the custom filesystem
  - `find()`: Locates files in the filesystem efficiently
- **Emu Base Class**: Abstract foundation for all emulator implementations
  - Defines common audio/video parameters (frequency, format, dimensions)
  - `frame_sample_count()`: Calculates precise audio samples per video frame
  - `load()` and `head()`: Unified file loading utilities
- **Memory Management**: `map_file()`, `unmap_file()` handle flash memory mapping for ROMs larger than RAM
- **Cross-Platform Utilities**: Input mapping helpers and file format detection

#### **gui.cpp** - User Interface & Input Processing Layer  
**Primary Role**: Complete user interface system with file browsing and input handling

**Key Components:**
- **Overlay Class**: Advanced on-screen display system
  - `draw_char()`: Hardware-accelerated character rendering with color blitting
  - `frame()`: UI frame rendering with transparency and alpha blending effects
  - `set_colors()`: Dynamic color palette management for different systems
- **GUI Class**: Full-featured file browser and menu system
  - `read_directory()`: Scans filesystem for compatible ROM files with filtering
  - `draw_files()`, `draw_help()`, `draw_info()`: Multiple UI panel rendering
  - `key()`: Complete keyboard navigation and input processing
  - `update_video()`, `update_audio()`: Real-time UI updates at 60FPS
- **HID Integration**: 
  - `gui_hid()`: USB HID keyboard event parsing and translation
  - `keyboard()`: Maps HID scan codes to GUI navigation actions
- **Audio Feedback**: Click sounds and UI audio generation

#### **emu_nofrendo.cpp** - NES Emulation Implementation Layer
**Primary Role**: Complete NES system emulation using the Nofrendo engine

**Key Components:**
- **EmuNofrendo Class**: Full NES emulator inheriting from Emu base framework
  - `insert()`: NES ROM validation, loading, and Nofrendo engine initialization
  - `update()`: Executes one complete frame of NES emulation (CPU, PPU, APU)
  - `key()` and `hid()`: NES-specific controller input mapping and processing
  - `audio_buffer()`: Real-time NES APU audio processing and buffering
- **Color System**: 
  - `make_nes_palette()`: Generates accurate NTSC/PAL color lookup tables
  - `make_yuv_palette()`: Creates composite video color conversion tables
  - Pre-computed 3-phase and 4-phase color palettes for optimal performance
- **Input Mapping**: Comprehensive controller support
  - `_common_nes[]`, `_classic_nes[]`, `_generic_nes[]`: Multiple controller layouts
  - D-pad, A/B buttons, Start/Select, and special function mappings
- **Nofrendo Integration**: Seamless bridge between ESP32 hardware and NES core
  - Handles memory management, timing synchronization, and hardware abstraction
  - Processes all NES subsystems: 6502 CPU, PPU graphics, 5-channel APU audio

## Current State & Known Issues

### Working Components
- ✅ Complete NES/Atari/SMS emulation cores
- ✅ ESP32 hardware abstraction (I2S, DAC, PWM)
- ✅ SPIFFS file system integration
- ✅ Audio processing pipeline (APU → PWM output)
- ✅ Build system (ESP-IDF components)

### Issues Requiring Attention  
- ❌ **GUI crashes**: NULLpointer access in video buffer (`_lines`) 
- ❌ **Input system**: HID stubs need proper integration or removal
- ❌ **mruby integration**: Missing despite project name
- ❌ **Video output**: Requires proper GPIO/DAC setup for composite output

### Development Notes
- **ESP_PLATFORM**: Auto-defined by ESP-IDF, enables ESP32-specific code paths
- **Bluetooth HID**: Completely removed/stubbed for APU-focused builds
- **Multi-core**: Emulation runs on Core 0, system tasks on Core 1
- **Audio latency**: ~1ms buffering with 1024-sample circular buffer

## Configuration Files

- `sdkconfig.defaults` - ESP32 project configuration (Bluetooth disabled)
- `partitions.csv` - Flash layout: 256KB SPIFFS, 1.7MB app partition
- `components/apu_emu/CMakeLists.txt` - Component build configuration
- Video: GPIO25 (composite), Audio: GPIO18 (PWM+RC filter), IR: GPIO0

## Key Technical Details

**Video System**: Generates NTSC (228 color clocks/line) or PAL (284 color clocks/line) composite video using I2S DMA and hardware DAC. Supports multiple pixel-to-colorclock ratios for different emulated systems.

**APU Timing**: NES APU runs at CPU clock/12 (~179kHz internal), outputs samples at ~15.7kHz for NTSC, ~15.6kHz for PAL. Each audio frame generates exactly the right number of samples for 60Hz/50Hz video synchronization.

**ROM Storage**: Uses custom CrapFS for large ROM files, automatically copies from SPIFFS to SPI flash partition with memory mapping for zero-copy access.

**Build Requirements**: ESP-IDF v5.4+, Docker container provided, requires ESP32 (not S2/S3/C3) for full hardware compatibility.

## Missing Implementations

- **mruby Integration**: No Ruby scripting despite project name
- **APU-focused Features**: Full emulator present, not stripped to audio-only
- **Modern Input**: Current HID system is complex; could benefit from simpler GPIO buttons
- **Audio Visualization**: APU analysis tools for development/debugging