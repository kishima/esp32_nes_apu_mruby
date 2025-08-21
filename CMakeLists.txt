cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nes-apu-mruby)

# Generate SPIFFS image from spiffs_data directory
spiffs_create_partition_image(audio spiffs_data FLASH_IN_PROJECT)

fatfs_create_rawflash_image(storage fatfs FLASH_IN_PROJECT PRESERVE_TIME)
