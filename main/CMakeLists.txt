idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES spi_flash
                       REQUIRES apu_emu picoruby-esp32)

target_compile_options(__idf_main PRIVATE 
    -Wno-error  # Don't treat warnings as errors
    -Wno-xor-used-as-pow  # Disable XOR-as-power warnings
    -Wno-volatile  # Disable volatile warnings
    -Wno-implicit-fallthrough  # Disable fallthrough warnings
    -Wno-attributes  # Disable attribute warnings
)