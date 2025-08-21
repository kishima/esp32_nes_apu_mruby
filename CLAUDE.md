# CLAUDE.md

## Build System

**Primary Build Method:**
```bash
./.devcontainer/run_devcontainer.sh idf.py build
./.devcontainer/run_devcontainer.sh idf.py flash
./.devcontainer/run_devcontainer.sh idf.py monitor
```

## Flash bin

```bash
./.devcontainer/run_devcontainer.sh esptool.py -b 460800 erase_region 0x200000 0x100000
./.devcontainer/run_devcontainer.sh esptool.py -b 460800 write_flash 0x200000 build/storage.bin
```
