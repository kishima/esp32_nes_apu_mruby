idf_component_register(
    SRCS "src/emu.cpp" 
         "src/emu_nofrendo.cpp" 
         "src/gui.cpp"
         "src/hid.cpp"
         "src/nofrendo/bitmap.c"
         "src/nofrendo/config.c"
         "src/nofrendo/event.c"
         "src/nofrendo/fds_snd.c"
         "src/nofrendo/gui.c"
         "src/nofrendo/gui_elem.c"
         "src/nofrendo/libsnss.c"
         "src/nofrendo/log.c"
         "src/nofrendo/map000.c"
         "src/nofrendo/map001.c"
         "src/nofrendo/map002.c"
         "src/nofrendo/map003.c"
         "src/nofrendo/map004.c"
         "src/nofrendo/map005.c"
         "src/nofrendo/map007.c"
         "src/nofrendo/map008.c"
         "src/nofrendo/map009.c"
         "src/nofrendo/map011.c"
         "src/nofrendo/map015.c"
         "src/nofrendo/map016.c"
         "src/nofrendo/map018.c"
         "src/nofrendo/map019.c"
         "src/nofrendo/map024.c"
         "src/nofrendo/map032.c"
         "src/nofrendo/map033.c"
         "src/nofrendo/map034.c"
         "src/nofrendo/map040.c"
         "src/nofrendo/map041.c"
         "src/nofrendo/map042.c"
         "src/nofrendo/map046.c"
         "src/nofrendo/map050.c"
         "src/nofrendo/map064.c"
         "src/nofrendo/map065.c"
         "src/nofrendo/map066.c"
         "src/nofrendo/map070.c"
         "src/nofrendo/map073.c"
         "src/nofrendo/map075.c"
         "src/nofrendo/map078.c"
         "src/nofrendo/map079.c"
         "src/nofrendo/map085.c"
         "src/nofrendo/map087.c"
         "src/nofrendo/map093.c"
         "src/nofrendo/map094.c"
         "src/nofrendo/map099.c"
         "src/nofrendo/map160.c"
         "src/nofrendo/map229.c"
         "src/nofrendo/map231.c"
         "src/nofrendo/mapvrc.c"
         "src/nofrendo/memguard.c"
         "src/nofrendo/mmc5_snd.c"
         "src/nofrendo/mmclist.c"
         "src/nofrendo/nes.c"
         "src/nofrendo/nes6502.c"
         "src/nofrendo/nes_apu.c"
         "src/nofrendo/nes_mmc.c"
         "src/nofrendo/nes_pal.c"
         "src/nofrendo/nes_ppu.c"
         "src/nofrendo/nes_rom.c"
         "src/nofrendo/nesinput.c"
         "src/nofrendo/nesstate.c"
         "src/nofrendo/nofrendo.c"
         "src/nofrendo/osd.c"
         "src/nofrendo/vid_drv.c"
         "src/nofrendo/vrcvisnd.c"
    INCLUDE_DIRS 
        "src"
        "src/nofrendo"
    REQUIRES 
        "spiffs" 
        "driver" 
        "esp_partition"
        "esp_timer"
        "freertos"
        "esp_system"
        "soc"
        "esp_hw_support"
        "spi_flash"
)

# target_compile_definitions(${COMPONENT_LIB} PRIVATE
#     -DNOFCONFIG_H
#     -DNES_SMALL_STACK
#     -DESPWIFI
# )
target_compile_options(__idf_apu_emu PRIVATE 
    -w  # Disable all warnings
    -Wno-error  # Don't treat warnings as errors
    -Wno-xor-used-as-pow  # Disable XOR-as-power warnings
    -Wno-volatile  # Disable volatile warnings
    -Wno-implicit-fallthrough  # Disable fallthrough warnings
    -Wno-attributes  # Disable attribute warnings
    -Wno-missing-field-initializers  # Disable struct initializer warnings
    -Wno-deprecated-declarations  # Disable deprecated API warnings
)