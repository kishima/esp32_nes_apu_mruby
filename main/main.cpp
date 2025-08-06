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

#include "emu.h"

// esp_8_bit
#define VIDEO_STANDARD NTSC
#define EMULATOR EMU_NES

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

// Global variables for video system (stubs)
volatile int _frame_counter = 0;
uint32_t _blit_ticks_min = 0xFFFFFFFF;
uint32_t _blit_ticks_max = 0;
uint32_t _isr_us = 0;
uint8_t** _lines = nullptr;

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


extern "C" void app_main(void)
{ 
  mount_filesystem();                       // mount the filesystem!
  _emu = NewNofrendo(VIDEO_STANDARD);

  if (!_emu) {
    printf("Failed to create emulator!\n");
    return;
  }
  
  xTaskCreatePinnedToCore(emu_task, "emu_task", 5*1024, NULL, 0, NULL, 0); // nofrendo needs 5k word stack, start on core 0

  // Main loop (replaces Arduino loop())
  while(1) {
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
    // Small delay to prevent watchdog timeout
    vTaskDelay(1);
  }
}