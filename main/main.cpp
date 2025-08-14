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
int _sample_count = -1;
//uint32_t _frame_time = 0;
// uint32_t _drawn = 1;

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


void update_audio()
{
    // _emu->update();  // コメントアウト

    int16_t abuffer[313*2];
    int format = _emu->audio_format >> 8;
    _sample_count = _emu->frame_sample_count();
    
    // 簡単なテスト音波形を生成
    static int test_counter = 0;
    for (int i = 0; i < _sample_count; i++) {
        // 低い周波数の矩形波テスト (約100Hz)
        if ((test_counter / 78) % 2 == 0) {  // 15700Hz / 78 / 2 ≈ 100Hz
            abuffer[i] = 10000;   // 正の値
        } else {
            abuffer[i] = -10000;  // 負の値
        }
        test_counter++;
    }
    printf("TEST: generating square wave, counter=%d\n", test_counter);
    
    // _sample_count = _emu->audio_buffer(abuffer,sizeof(abuffer));  // コメントアウト
    
    // オーディオデバッグ：60フレームごとにチェック
    static uint32_t audio_frame_count = 0;
    if (audio_frame_count % 60 == 0) {
        // サンプル数と最初のいくつかのサンプル値を表示
        printf("AUDIO[%lu]: samples=%d, format=%d\n", audio_frame_count, _sample_count, format);
        
        if (_sample_count > 0) {
            printf("AUDIO: first 8 samples: ");
            for (int i = 0; i < 8 && i < _sample_count; i++) {
                printf("0x%04X ", (uint16_t)abuffer[i]);
            }
            printf("\n");
        }
    }
    audio_frame_count++;
    
    printf("TEST: calling audio_write_16 with samples=%d, format=%d\n", _sample_count, format);
    audio_write_16(abuffer,_sample_count,1);  // 強制的にモノラル(1チャンネル)に設定
}

// dual core mode runs emulator on comms core
void emu_task(void* arg)
{
    uint32_t cpu_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
    printf("emu_task %s running on core %d at %lu MHz\n",
      _emu->name.c_str(), xPortGetCoreID(), cpu_freq_mhz);
    printf("CPU Frequency: %lu MHz\n", cpu_freq_mhz);

    //emu init
    //std::string rom_file = "/" + _emu->name + "/chase.nes";
    std::string rom_file = "/nsf/test.nsf";
    if (_emu->insert(rom_file.c_str(),0,0) != 0) {
        printf("Failed to load ROM, suspending emu_task\n");
        vTaskSuspend(NULL);  // Suspend this task to prevent crashes
        return;
    }
    //_drawn = _frame_counter;

    // 60Hz timing constants
    const uint64_t target_frame_time_us = 16667;  // 60Hz = 16.67ms
    uint64_t next_frame_time = esp_timer_get_time();
    uint32_t frame_count = 0;
    uint32_t total_processing_time = 0;
    
    printf("Starting 60Hz NSF playback loop...\n");

    while(true) //emu loop
    {
      uint64_t frame_start = esp_timer_get_time();
      
      // NSF audio processing
      update_audio();
      
      uint64_t frame_end = esp_timer_get_time();
      uint32_t processing_time_us = (uint32_t)(frame_end - frame_start);
      total_processing_time += processing_time_us;
      frame_count++;
      
      // Calculate next frame time
      next_frame_time += target_frame_time_us;
      
      // Sleep until next frame
      int64_t sleep_time_us = next_frame_time - frame_end;
      //printf("next_frame_time:%lld,processing_time_us:%ld, sleep_time_us=%lld\n",next_frame_time,processing_time_us,sleep_time_us);
      
      if (sleep_time_us > 1000) {
        // Sleep if we have more than 1ms left
        vTaskDelay(pdMS_TO_TICKS(sleep_time_us / 1000));
      } else if (sleep_time_us < 0) {
        // If we're more than one frame behind, reset timing
        printf("Frame timing reset - processing took too long %lld\n",sleep_time_us);
        next_frame_time = esp_timer_get_time();
      }
      
      // Performance logging every 5 seconds (300 frames)
      if (frame_count % 300 == 0) {
        uint32_t avg_processing_us = total_processing_time / 300;
        float cpu_usage = (float)avg_processing_us / target_frame_time_us * 100.0f;
        printf("NSF 60Hz: avg processing=%lu us, CPU usage=%.1f%%, frame=%lu\n", 
               avg_processing_us, cpu_usage, frame_count);
        total_processing_time = 0;
      }
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
  //TODO: update for audio
  #if 0
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
  #endif
}
#else
void perf(){};
#endif

extern "C" void app_main(void)
{    
  printf("app_main on core %d\n", xPortGetCoreID()); 
  mount_filesystem();                       // mount the filesystem!
  //_emu = NewNofrendo(VIDEO_STANDARD);       // create the emulator!
  _emu = NewNsfplayer(VIDEO_STANDARD);       // create the emulator!

  // create for Emulator task
  // nofrendo needs 5k word stack, start on core 1
  xTaskCreatePinnedToCore(emu_task, "emu_task", 5*1024, NULL, 4, NULL, 1);
  
  while(true){
    // start the video after emu has started
    if (_sample_count >= 0) { 
      printf("audio/video_init\n");
      video_init(_emu->cc_width,_emu->flavor,_emu->composite_palette(),_emu->standard); // start the A/V pump
      break;
    }
    vTaskDelay(1);
  }
  printf("emulator gets started. video_init done\n");

  //TODO: exec mruby here

  while(true){
    vTaskDelay(100);
    // Dump some stats
    perf();
  }
}
