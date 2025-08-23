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
#include "string.h"
#include "nofrendo/noftypes.h"
#include "nofrendo/nes_apu.h"
#include "picoruby-esp32.h"
#include "apu_if.h"

#include <stdint.h>
#include <stdbool.h>

}

// デバッグログ制御フラグ
#define AUDIO_DEBUG 0  // オーディオフレーム詳細ログ

#include "emu.h"
#include "video_out.h"

#define NTSC_SAMPLE 262

int _sample_count = -1;
static uint8_t* _input_data;

static apu_log_header_t _apulog_header;
static apu_log_entry_t* _apulog_entries;
static int _apu_init = 0;
static int _frame_count = 0;
static int _entry_count = 0;
static int _play_head = 0;

int exec_seek_play_head(){
  for (uint32_t i = 0; i < _apulog_header.entry_count; i++) {
    const apu_log_entry_t* entry = &_apulog_entries[i];
    if(entry->event_type == APU_EVENT_INIT_END){
      return i+1;
    }
  }
  return -1;
}

void exec_init_entries(){
  _play_head = exec_seek_play_head();
  if(_play_head < 0){
    printf("PLAY entry not found\n");
    return;
  }
  for (uint32_t i = 0; i < _play_head; i++) {
    const apu_log_entry_t* entry = &_apulog_entries[i];
    if(!entry) return;
    switch (entry->event_type) {
      case APU_EVENT_WRITE:
        apuif_write_reg(entry->addr, entry->data);
        _frame_count = entry->frame_number;
        //printf("%8lu 0x%04X 0x%02X\n",i , entry->addr, entry->data);
        break;
      case APU_EVENT_INIT_START:
      case APU_EVENT_INIT_END:
      case APU_EVENT_PLAY_START:
      case APU_EVENT_PLAY_END:
      default:
        break;
    }
  }
  _entry_count = _play_head;
}

void exec_play_entries(){
  //printf("start entry_count=%d\n",_entry_count);
  for (uint32_t i = _entry_count + 1; i < _apulog_header.entry_count ; i++) {
    const apu_log_entry_t* entry = &_apulog_entries[i];
    if(!entry) return;
    switch (entry->event_type) {
      case APU_EVENT_WRITE:
        apuif_write_reg(entry->addr, entry->data);
        //printf("%8lu 0x%04X 0x%02X %8lu\n",i , entry->addr, entry->data, entry->frame_number);
        break;
      case APU_EVENT_PLAY_START:
        _entry_count = i;
        //printf("end entry_count=%d\n",_entry_count);
        return;
        break;
      case APU_EVENT_PLAY_END:
        _entry_count = i+1;
        //printf("end entry_count=%d\n",_entry_count);
        return;
        break;
      case APU_EVENT_INIT_START:
      case APU_EVENT_INIT_END:
      default:
        printf("unexpected event %d\n",entry->event_type);
        break;
    }
  }
  //loop
  _entry_count = _play_head;
}

void update_audio()
{
  if(!_apu_init){
    exec_init_entries();
    _apu_init = 1;
  }

  exec_play_entries();

  int16_t abuffer[(NTSC_SAMPLE+1)*2];
  memset(abuffer,0,sizeof(abuffer));
  _sample_count = apuif_frame_sample_count();
  
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
#else
  _sample_count = apuif_process(abuffer,sizeof(abuffer));
#endif
  
  if (AUDIO_DEBUG) {
    // オーディオデバッグ：60フレームごとにチェック
    static uint32_t audio_frame_count = 0;
    if (audio_frame_count % 60 == 0) {
        // サンプル数と最初のいくつかのサンプル値を表示
        printf("AUDIO[%lu]: samples=%d\n", audio_frame_count, _sample_count);
        
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
    
  audio_write_16(abuffer,_sample_count,1);
}

esp_err_t mount_filesystem()
{
  printf("\n\n\nesp_8_bit\n\nmounting spiffs (will take ~15 seconds if formatting for the first time)....\n");
  uint32_t t = esp_timer_get_time() / 1000;
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/audio",
    .partition_label = "audio",
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

uint8_t* load_file_from_spiffs(const char* filename, int* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return nullptr;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory and read file
    uint8_t* file_data = (uint8_t*)malloc(*size);
    if (!file_data) {
        printf("Failed to allocate memory for the file\n");
        fclose(file);
        return nullptr;
    }

    size_t read_bytes = fread(file_data, 1, *size, file);
    fclose(file);

    if (read_bytes != *size) {
        printf("Failed to read complete file\n");
        free(file_data);
        return nullptr;
    }

    printf("Loaded file %s: %d bytes\n", filename, *size);
    return file_data;
}

void emu_task(void* arg)
{
  printf("emu_task on core %d\n", xPortGetCoreID());
  uint32_t cpu_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
  apuif_init();
  printf("CPU Frequency: %lu MHz\n", cpu_freq_mhz);

  // 乱数シードの初期化
  srand(esp_timer_get_time());

  mount_filesystem(); //mount the filesystem!

  _apulog_entries = apuif_read_entries("/audio/nsf_local/apu_log_track0.bin", &_apulog_header);

  int file_size = 0;
  const char* filename = "/audio/nsf/test.nsf";
  _input_data = load_file_from_spiffs(filename, &file_size);
  printf("file(%s) is loaded(%d)\n", filename, file_size);

  //video_init(_emu->cc_width,_emu->flavor,_emu->composite_palette(),_emu->standard); // start the A/V pump
  video_init(4,2,NULL,1); // start the A/V pump

  // 60Hz timing constants
  const uint64_t target_frame_time_us = 16667;  // 60Hz = 16.67ms
  uint64_t next_frame_time = esp_timer_get_time();
  uint32_t frame_count = 0;
  uint32_t total_processing_time = 0;
  
  printf("Starting 60Hz NSF playback loop...\n");

  while(true) //emu loop
  {
    uint64_t frame_start = esp_timer_get_time();
    update_audio();
  
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

extern "C" void app_main(void)
{    
  printf("app_main on core %d\n", xPortGetCoreID());
  xTaskCreatePinnedToCore(emu_task, "emu_task", 5*1024, NULL, 4, NULL, 1);
  
  while(true){
    //wait audio setup
    vTaskDelay(1);
  }
  printf("emulator gets started. video_init done\n");

  printf("start picoruby-esp32\n");
  picoruby_esp32();
  printf("end picoruby-esp32\n");
}
