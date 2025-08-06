/* Copyright (c) 2020, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "esp_system.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <stdio.h>
#include <inttypes.h>
#include <algorithm>
#include "soc/rtc.h"

#define PERF  // some stats about where we spend our time
#include "emu.h"
// Temporarily disable video_out.h to focus on APU functionality
// #include "video_out.h"

// esp_8_bit
// Atari 8 computers, NES and SMS game consoles on your TV with nothing more than a ESP32 and a sense of nostalgia
// Supports NTSC/PAL composite video, Bluetooth Classic keyboards and joysticks

//  Choose one of the video standards: PAL,NTSC
#define VIDEO_STANDARD NTSC

//  Choose one of the following emulators: EMU_NES,EMU_SMS,EMU_ATARI
#define EMULATOR EMU_NES  // Changed to EMU_NES as EMU_ATARI functions are not available

//  Many emus work fine on a single core (S2), file system access can cause a little flickering
//  #define SINGLE_CORE

// The filesystem should contain folders named for each of the emulators i.e.
//    atari800
//    nofrendo
//    smsplus
// Folders will be auto-populated on first launch with a built in selection of sample media.
// Use 'ESP32 Sketch Data Upload' from the 'Tools' menu to copy a prepared data folder to ESP32

// Note: gui_start and gui_update are implemented in gui.cpp component

void video_sync() {
    // Stub: Video sync functionality
}

void video_init(int cc_width, int flavor, const void* palette, int standard) {
    printf("Video initialized: cc_width=%d, flavor=%d, standard=%d\n", cc_width, flavor, standard);
}

// Stub for make_yuv_palette function (normally in video_out.h)
void make_yuv_palette(const char* name, const uint32_t* palette, int mode) {
    printf("YUV palette created: %s, mode=%d\n", name, mode);
}

// Audio output stub (APU output will go here later)
void audio_write_16(const int16_t* samples, int channels, int length) {
    // TODO: This is where APU audio output will be implemented
    // For now, just discard the audio data
    static int call_count = 0;
    if (call_count++ < 10) {
        printf("Audio output: %d samples, %d channels, length=%d\n", 
               samples ? 1 : 0, channels, length);
    }
}

// System preferences stubs (normally stored in NVS)
int sys_get_pref(const char* key, char* value, int max_len) {
    // Return empty/default values for preferences
    if (value && max_len > 0) {
        value[0] = 0;  // Empty string
    }
    return 0;
}

void sys_set_pref(const char* key, const char* value) {
    printf("Setting preference: %s = %s\n", key, value);
}

// IR input stub
int get_hid_ir(uint8_t* data) {
    // No IR input for now
    return 0;
}

// Global variables for video system (stubs)
volatile int _frame_counter = 0;
uint32_t _blit_ticks_min = 0xFFFFFFFF;
uint32_t _blit_ticks_max = 0;
uint32_t _isr_us = 0;
uint8_t** _lines = nullptr;

// Create a new emulator, messy ifdefs ensure that only one links at a time
Emu* NewEmulator()
{  
  #if (EMULATOR==EMU_NES)
  return NewNofrendo(VIDEO_STANDARD);
  #endif
  #if (EMULATOR==EMU_SMS)
  // return NewSMSPlus(VIDEO_STANDARD);  // Not available in current build
  printf("SMS emulator not available in current build\n");
  return nullptr;
  #endif
  #if (EMULATOR==EMU_ATARI)
  // return NewAtari800(VIDEO_STANDARD);  // Not available in current build
  printf("Atari emulator not available in current build\n");
  return nullptr;
  #endif
  printf("Must choose one of the following emulators: EMU_NES,EMU_SMS,EMU_ATARI\n");
  return nullptr;
}

Emu* _emu = 0;            // emulator running on core 0
uint32_t _frame_time = 0;
uint32_t _drawn = 1;
bool _inited = false;

void emu_init()
{
    std::string folder = "/" + _emu->name;
    gui_start(_emu, folder.c_str());
    _drawn = _frame_counter;
}

void emu_loop()
{
    // wait for blanking before drawing to avoid tearing
    video_sync();

    // Draw a frame, update sound, process hid events
    uint32_t t = xthal_get_ccount();
    gui_update();
    _frame_time = xthal_get_ccount() - t;
    _lines = _emu->video_buffer();
    _drawn++;
}

// dual core mode runs emulator on comms core
void emu_task(void* arg)
{
    printf("emu_task %s running on core %d\n",
      _emu->name.c_str(), xPortGetCoreID());
    emu_init();
    for (;;)
      emu_loop();
}

esp_err_t mount_filesystem()
{
  printf("\n\n\nesp_8_bit\n\nmounting spiffs (will take ~15 seconds if formatting for the first time)....\n");
  uint32_t t = esp_timer_get_time() / 1000;
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true  // force?
  };
  esp_err_t e = esp_vfs_spiffs_register(&conf);
  if (e != 0)
    printf("Failed to mount or format filesystem: %d. Use 'ESP32 Sketch Data Upload' from 'Tools' menu\n",e);
  vTaskDelay(1);
  printf("... mounted in %" PRIu32 " ms\n", (uint32_t)(esp_timer_get_time() / 1000) - t);
  return e;
}

#ifdef PERF
void perf()
{
  static int _next = 0;
  if (_drawn >= _next) {
    float elapsed_us = 120*1000000/(_emu->standard ? 60 : 50);
    _next = _drawn + 120;
    
    printf("frame_time:%lu drawn:%" PRIu32 " displayed:%d blit_ticks:%" PRIu32 "->%" PRIu32 ", isr time:%2.2f%%\n",
      _frame_time/240, _drawn, _frame_counter, _blit_ticks_min, _blit_ticks_max, (_isr_us*100)/elapsed_us);
      
    _blit_ticks_min = 0xFFFFFFFF;
    _blit_ticks_max = 0;
    _isr_us = 0;
  }
}
#else
void perf(){};
#endif

extern "C" void app_main(void)
{ 
  // rtc_clk_cpu_freq_set(RTC_CPU_FREQ_240M);  // CPU freq setting not available in ESP-IDF v5.4
  mount_filesystem();                       // mount the filesystem!
  _emu = NewEmulator();                     // create the emulator!
  
  if (!_emu) {
    printf("Failed to create emulator!\n");
    return;
  }
  
  int hid_result = hid_init("emu32");       // bluetooth hid on core 1!
  if (hid_result != 0) {
    printf("HID initialization failed: %d\n", hid_result);
  }

  #ifdef SINGLE_CORE
  emu_init();
  video_init(_emu->cc_width, _emu->flavor, _emu->composite_palette(), _emu->standard); // start the A/V pump on app core
  #else
  xTaskCreatePinnedToCore(emu_task, "emu_task", EMULATOR == EMU_NES ? 5*1024 : 3*1024, NULL, 0, NULL, 0); // nofrendo needs 5k word stack, start on core 0
  #endif

  // Main loop (replaces Arduino loop())
  while(1) {
    #ifdef SINGLE_CORE
    emu_loop();
    #else
    // start the video after emu has started
    if (!_inited) {
      if (_lines) {
        printf("video_init\n");
        video_init(_emu->cc_width, _emu->flavor, _emu->composite_palette(), _emu->standard); // start the A/V pump
        _inited = true;
      } else {
        vTaskDelay(1);
      }
    }
    #endif
    
    // update the bluetooth edr/hid stack
    hid_update();

    // Dump some stats
    perf();
    
    // Small delay to prevent watchdog timeout
    vTaskDelay(1);
  }
}