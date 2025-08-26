# esp32_nes_apu_mruby

## Features

A project for playing NES (Famicom) APU sound sources using PicoRuby on ESP32.

## Target Device

Configured for M5StickC plus2.

PSRAM is mandatory.

While other ESP32 series with PSRAM should work by adjusting Flash and PSRAM sizes, this has not been verified.

### Required External Devices

An I2S slave device is required.
Tested with PCM5102.
You can also use PWM output by commenting out `#define USE_I2S` in apu_emu.

### Wiring

Pin configuration as follows. Can be changed by editing `apu_if.h`.

```
#define PIN_BCK   GPIO_NUM_26
#define PIN_WS    GPIO_NUM_25
#define PIN_DOUT  GPIO_NUM_33
```

## Building for ESP32

A .devcontainer folder is provided, but hasn't been tested as a devcontainer. I use the following commands for testing:

### First Time Setup

```bash
cd .devcontainer
./build.sh
./.devcontainer/run_devcontainer.sh idf.py set-target esp32
```

### Regular Build

```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
```

## Usage

Place binary files in the format specified in `logformat.txt` under `fatfs/home/` and use the following command from the R2P2 shell:

Binary files should have the `.reglog` extension.

```
$> play sample
```

## Credits

The sample.reglog file was generated from the following NSF file:

Original work: "crimmy buzz.nsf" by big lumby, CC BY-NC-SA 3.0

https://battleofthebits.com/arena/Entry/crimmy+buzz.nsf/50518/
