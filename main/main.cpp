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
#include "video_out.h"

// esp_8_bit
//  Choose one of the video standards: PAL,NTSC
#define VIDEO_STANDARD NTSC

Emu* _emu = 0;
uint32_t _frame_time = 0;
uint32_t _drawn = 1;
bool _inited = false;

using namespace std;

string get_ext(const string& s)
{
    string ext;
    auto i = s.find_last_of(".");
    if (i > 0) {
        ext = s.substr(i+1);
        for (i = 0; i < ext.length(); i++)
            ext[i] = tolower(ext[i]);
    }
    return ext;
}

// missing in arduino?
string to_string(int i)
{
    char buf[32];
    sprintf(buf,"%d",i);
    return buf;
}


void update_av()
{
    _emu->update();

    int16_t abuffer[313*2];
    int format = _emu->audio_format >> 8;
    int sample_count = _emu->frame_sample_count();
    sample_count = _emu->audio_buffer(abuffer,sizeof(abuffer));
    audio_write_16(abuffer,sample_count,format);
}

// dual core mode runs emulator on comms core
void emu_task(void* arg)
{
    uint32_t cpu_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
    printf("emu_task %s running on core %d at %lu MHz\n",
      _emu->name.c_str(), xPortGetCoreID(), cpu_freq_mhz);
    printf("CPU Frequency: %lu MHz\n", cpu_freq_mhz);

    //emu init
    std::string rom_file = "/" + _emu->name + "/chase.nes";
    if (_emu->insert(rom_file.c_str(),0,0) != 0) {
        printf("Failed to load ROM, suspending emu_task\n");
        vTaskSuspend(NULL);  // Suspend this task to prevent crashes
        return;
    }
    _drawn = _frame_counter;

    while(true) //emu loop
    {
      // wait for blanking before drawing to avoid tearing
      video_sync();
      // Draw a frame, update sound, process hid events
      uint32_t t = xthal_get_ccount();
      update_av();
      _frame_time = xthal_get_ccount() - t;
      _lines = _emu->video_buffer();
      _drawn++;
    }
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
    
    printf("frame_time:%lu drawn:%lu displayed:%d blit_ticks:%lu->%lu, isr time:%2.2f%%\n",
      _frame_time/240,_drawn,_frame_counter,_blit_ticks_min,_blit_ticks_max,(_isr_us*100)/elapsed_us);
      
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
  printf("app_main on core %d\n", xPortGetCoreID()); 
  mount_filesystem();                       // mount the filesystem!
  _emu = NewNofrendo(VIDEO_STANDARD);       // create the emulator!

  // create for Emulator task
  // nofrendo needs 5k word stack, start on core 1
  xTaskCreatePinnedToCore(emu_task, "emu_task", 5*1024, NULL, 4, NULL, 1);
  
  while(true){
    // start the video after emu has started
    if (!_inited) {
      if (_lines) {
        printf("video_init\n");
        video_init(_emu->cc_width,_emu->flavor,_emu->composite_palette(),_emu->standard); // start the A/V pump
        _inited = true;
        printf("video_init done\n");
      } else {
        vTaskDelay(1);
      }
    }

    // 擬似的にキー入力を与える（テスト用）- 実時間ベース
    #if 1
    static uint32_t last_key_time = 0;
    static bool key_pressed = false;
    uint32_t current_time = esp_timer_get_time() / 1000; // ミリ秒単位
    
    if (current_time - last_key_time >= 2000) {  // 2秒間隔
        if (!key_pressed) {
            _emu->key(40, 1, 0);  // Startボタン（Return）押下
            //printf("Simulated Start button press at %ld ms\n", current_time);
            key_pressed = true;
        } else {
            _emu->key(40, 0, 0);  // Startボタン離す
            //printf("Simulated Start button release at %ld ms\n", current_time);
            key_pressed = false;
        }
        last_key_time = current_time;
    }
    #endif
    
    vTaskDelay(100);

    // Dump some stats
    perf();
  }
}
