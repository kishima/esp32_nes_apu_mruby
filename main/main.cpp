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
#include <stdlib.h>
#include "soc/rtc.h"

extern "C" {
#include "picoruby-esp32.h"
}

// デバッグログ制御フラグ
#define AUDIO_DEBUG 0  // オーディオフレーム詳細ログ

#include "emu.h"
#include "video_out.h"

// esp_8_bit
#define VIDEO_STANDARD NTSC

Emu* _emu = 0;
int _sample_count = -1;

using namespace std;

#define NTSC_SAMPLE 262

void update_audio()
{
    // 安全性チェック：エミュレータポインタの検証
    if (!_emu) {
        printf("[AUDIO_ERROR] Emulator pointer is NULL in update_audio()\n");
        return;
    }
    
    // NSFプレイヤーのupdate()実行 - 内部でAPU処理も行う
    _emu->update();  // PLAYルーチンを実行

    int16_t abuffer[(NTSC_SAMPLE+1)*2];
    int format = _emu->audio_format >> 8;
    _sample_count = _emu->frame_sample_count();
    
    // サンプル数の検証
    if (_sample_count <= 0 || _sample_count > (NTSC_SAMPLE+1)*2) {
        printf("[AUDIO_ERROR] Invalid sample count: %d\n", _sample_count);
        return;
    }
    
#if 0
      // ランダムなテスト音波形を生成
      for (int i = 0; i < _sample_count; i++) {
          // -10000 から 10000 の範囲でランダムな値を生成
          abuffer[i] = (rand() % 20001) - 10000;
      }
#endif

#if 0
    // 手動矩形波生成 (440Hz)
    static uint32_t sample_counter = 0;
    static int wave_state = 1;
    
    // NTSC: 15720Hz sample rate, 440Hz = 15720/440 = 35.7 samples per cycle
    const int samples_per_cycle = 36;  // 約440Hz
    const int16_t amplitude = 8000;    // APU相当の振幅
    
    for (int i = 0; i < _sample_count; i++) {
        // 矩形波: half cycle high, half cycle low
        if ((sample_counter % samples_per_cycle) < (samples_per_cycle / 2)) {
            abuffer[i] = amplitude;
        } else {
            abuffer[i] = -amplitude;
        }
        sample_counter++;
    }
#else

    _sample_count = _emu->audio_buffer(abuffer,sizeof(abuffer));

#endif

    
    if (AUDIO_DEBUG) {
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
    }
    
    //printf("TEST: calling audio_write_16 with samples=%d, format=%d\n", _sample_count, format);
    audio_write_16(abuffer,_sample_count,1);  // 強制的にモノラル(1チャンネル)に設定
}

// dual core mode runs emulator on comms core
void emu_task(void* arg)
{
    uint32_t cpu_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
    printf("emu_task %s running on core %d at %lu MHz\n",
      _emu->name.c_str(), xPortGetCoreID(), cpu_freq_mhz);
    printf("CPU Frequency: %lu MHz\n", cpu_freq_mhz);
    
    // 乱数シードの初期化
    srand(esp_timer_get_time());

    //emu init
    //std::string rom_file = "/nsf/continuous_tone_single.nsf";
    //std::string rom_file = "/nsf/test.nsf";
    //std::string rom_file = "/nsf/minimal_test.nsf";
    std::string rom_file = "/nsf_local/dq.nsf";
    //std::string rom_file = "/nsf_local/meikyu.nsf";
    if (_emu->insert(rom_file.c_str(),0,0) != 0) {
        printf("Failed to load ROM, suspending emu_task\n");
        vTaskSuspend(NULL);  // Suspend this task to prevent crashes
        return;
    }

    // 60Hz timing constants
    const uint64_t target_frame_time_us = 16667;  // 60Hz = 16.67ms
    //const uint64_t target_frame_time_us = 16639;  // 60.0988Hz = 16.639ms
    uint64_t next_frame_time = esp_timer_get_time();
    uint32_t frame_count = 0;
    uint32_t total_processing_time = 0;
    
    printf("Starting 60Hz NSF playback loop...\n");

    while(true) //emu loop
    {
      uint64_t frame_start = esp_timer_get_time();
      
      //printf("======== 60Hz Loop =========\n");
      // NSF audio processing
      update_audio();
      //printf("audio update done\n");

      uint32_t buffer_used = _audio_w - _audio_r;
      uint32_t buffer_free = sizeof(_audio_buffer) - buffer_used;

      // Audio buffer 詳細ログ (5秒間隔)
      static uint32_t detailed_log_frame = 0;
      if (detailed_log_frame % 300 == 0) {
          printf("RING_BUFFER[%lu]: w=%lu r=%lu used=%lu free=%lu\n", 
                 detailed_log_frame, _audio_w & 1023, _audio_r & 1023, buffer_used, buffer_free);
      }
      detailed_log_frame++;

      // Audio buffer 警告
      if (buffer_used < 100) printf("underflow %ld\n", buffer_used);
      if (buffer_used > 900) printf("overflow %ld\n", buffer_used);
      //printf("buf w:%ld r:%ld\n", _audio_w, _audio_r);

      uint64_t frame_end = esp_timer_get_time();
      uint32_t processing_time_us = (uint32_t)(frame_end - frame_start);
      total_processing_time += processing_time_us;
      frame_count++;
      
      // Calculate next frame time
      next_frame_time += target_frame_time_us;
      
      // Sleep until next frame
      int64_t sleep_time_us = next_frame_time - frame_end;
      
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

  printf("start picoruby-esp32\n");
  picoruby_esp32();
  printf("end picoruby-esp32\n");
}
