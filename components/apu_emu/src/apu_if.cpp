#include "apu_if.h"
#include "nofrendo/noftypes.h"
#include "nes_apu.h"
#include "esp_heap_caps.h"

#include "soc/rtc_io_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "rom/lldesc.h"
#include "driver/periph_ctrl.h"
#include "driver/dac.h"
#include "driver/gptimer.h"

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static apu_t* _apu = 0;
static int _audio_frequency = 0;
static int _audio_frame_samples = 0;
static int _audio_fraction = 0;
static int _initialized = 0;

uint8_t _audio_buffer[1024] __attribute__((aligned(4)));
uint32_t volatile _audio_r = 0;
uint32_t volatile _audio_w = 0;

static uint8_t last_s __attribute__((section(".noinit"))); 

#ifdef USE_I2S
#include "driver/i2s_std.h"
#include "driver/gpio.h"

void apuif_hw_init_i2c(){

}

static void audio_write_i2s(){

}

#else
#include "soc/ledc_struct.h"
#include "driver/ledc.h"

gptimer_handle_t _audio_timer = NULL;

inline void IRAM_ATTR audio_sample(uint8_t s)
{
    auto& reg = LEDC.channel_group[0].channel[0];
    reg.duty.duty = s << 4; // 25 bit (21.4)
    reg.conf0.sig_out_en = 1; // This is the output enable control bit for channel
    reg.conf1.duty_start = 1; // When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware
    reg.conf0.clk_en = 1;
}

void IRAM_ATTR audio_isr()
{
    uint32_t r = _audio_r;   // ローカルに取り込み
    uint32_t w = _audio_w;   // 1回だけ読む（スナップショット）

    uint8_t s;
    if(_audio_r < _audio_w){
        s = _audio_buffer[_audio_r++ & (sizeof(_audio_buffer)-1)];
        last_s = s;
    }else{
        s = last_s;
    } 
    audio_sample(s);

}

bool IRAM_ATTR audio_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    audio_isr();
    return false; // return false to indicate we're not yielding to a higher priority task
}

// Setup audio timer interrupt based on sample rate
esp_err_t setup_audio_timer(float sample_rate_mhz)
{
    // Calculate timer period from sample rate
    // sample_rate_mhz is in MHz, convert to microseconds
    uint64_t timer_period_us = (uint64_t)(1.0 / sample_rate_mhz);

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz resolution
        .intr_priority = 0,
        .flags = {
            .intr_shared = 0
        }
    };
    
    esp_err_t ret = gptimer_new_timer(&timer_config, &_audio_timer);
    if (ret != ESP_OK) return ret;

    gptimer_event_callbacks_t cbs = {
        .on_alarm = audio_timer_callback,
    };
    ret = gptimer_register_event_callbacks(_audio_timer, &cbs, NULL);
    if (ret != ESP_OK) return ret;

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = timer_period_us,
        .reload_count = 0,
        .flags = {
            .auto_reload_on_alarm = 1,
        }
    };
    ret = gptimer_set_alarm_action(_audio_timer, &alarm_config);
    if (ret != ESP_OK) return ret;

    ret = gptimer_enable(_audio_timer);
    if (ret != ESP_OK) return ret;

    return gptimer_start(_audio_timer);
}

void apuif_hw_init_ledc()
{
    // Use NTSC audio sample rate: ~15.7kHz
    printf("Use Timer Interrupt for PWM audio\n");    
    setup_audio_timer(0.0157); // 15.7kHz sample rate
    
    // ESP-IDF LEDC configuration for PWM audio (ESP-IDF v5.4 compatible)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_7_BIT,
        .timer_num        = LEDC_TIMER_0,
        //.freq_hz          = 2000000,  // 2MHz PWM frequency
        .freq_hz          = 625000,  // 625KHz PWM frequency // 625 khz is as fast as we go with 7 bitss
        .clk_cfg          = LEDC_USE_APB_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = AUDIO_PIN,
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void audio_write_16(const int16_t* s, int len, int channels)
{
    int b;
    while (len--) {
        if (_audio_w == (_audio_r + sizeof(_audio_buffer))){
            break;
        }
        if (channels == 2) {
            b = (s[0] + s[1]) >> 9;
            s += 2;
        } else {
            b = *s++ >> 8;
        }
        if (b < -32) b = -32;
        if (b > 31) b = 31;
        _audio_buffer[_audio_w++ & (sizeof(_audio_buffer)-1)] = b + 32;
    }
}
#endif

void apuif_init(){
    if(_initialized) return;

#ifdef USE_I2S
    apuif_hw_init_i2s();
    //can be change value according to I2S setting.
    _audio_frequency = 15720; //NTSC
    _audio_frame_samples = (_audio_frequency << 16)/60;   // fixed point sampler
    _audio_fraction = 0;

    _apu = apu_create(0, _audio_frequency, 60, 8);

#else
    apuif_hw_init_ledc();
    _audio_frequency = 15720; //NTSC
    _audio_frame_samples = (_audio_frequency << 16)/60;   // fixed point sampler
    _audio_fraction = 0;

    _apu = apu_create(0, _audio_frequency, 60, 8);

#endif
    _initialized = 1;
}

int apuif_frame_sample_count()
{
    int n = _audio_frame_samples + _audio_fraction;
    _audio_fraction = n & 0xFFFF;
    return n >> 16;
}

int apuif_process(int16_t* buff, int len)
{
    int n = apuif_frame_sample_count();
    if(n > len){
        printf("bad buffer size %d > %d\n",n,len);
        return -1;
    }

    //void apu_process(void *buffer, int num_samples)
    apu_process(buff,n);
    uint8_t* b8 = (uint8_t*)buff;
    for (int i = n-1; i >= 0; i--){
        buff[i] = (b8[i] ^ 0x80) << 8;  // turn it back into signed 16
    }
    return n;
}

void apuif_write_reg(uint32_t address, uint8_t value)
{
    apu_write(address, value);
}

uint8_t apuif_read_reg(uint32_t address)
{
    return apu_read(address);
}


void apuif_audio_write(const int16_t* s, int len, int channels){
#ifdef USE_I2S
    //TODO: impl audio buffer set to I2S DMA buffer
    audio_write_i2s();
#else
    audio_write_16(s, len, channels);
#endif


static const char* get_register_name(uint16_t addr) {
    static const char* reg_names[] = {
        "Pulse1_Vol", "Pulse1_Sweep", "Pulse1_Lo", "Pulse1_Hi",
        "Pulse2_Vol", "Pulse2_Sweep", "Pulse2_Lo", "Pulse2_Hi",
        "Tri_Linear", "Reserved", "Tri_Lo", "Tri_Hi",
        "Noise_Vol", "Reserved", "Noise_Lo", "Noise_Hi",
        "DMC_Freq", "DMC_Raw", "DMC_Start", "DMC_Len",
        "OAM_DMA", "Status", "Joypad1", "Joypad2"
    };
    
    if (addr >= 0x4000 && addr <= 0x4017) {
        return reg_names[addr - 0x4000];
    }
    return "Unknown";
}

apu_log_entry_t* apuif_read_entries(const char* filename, apu_log_header_t* header){
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    /* Read header */
    if (fread(header, sizeof(apu_log_header_t), 1, file) != 1) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(file);
        return NULL;
    }
    
    /* Verify magic */
    if (memcmp(header->magic, "APULOG\0\0", 8) != 0) {
        fprintf(stderr, "Error: Invalid file format (bad magic)\n");
        fclose(file);
        return NULL;
    }
    
    printf("=== APU Binary Log File ===\n");
    printf("File: %s\n", filename);
    printf("Format version: %u\n", header->version);
    printf("Entry count: %u\n", header->entry_count);
    printf("Frame count: %u\n", header->frame_count);
    printf("\n");
    
    if (header->entry_count == 0) {
        printf("No entries in log file.\n");
        fclose(file);
        return NULL;
    }
    
    /* Allocate memory for entries */
    //apu_log_entry_t* entries = malloc(header.entry_count * sizeof(apu_log_entry_t));
    apu_log_entry_t* entries = (apu_log_entry_t*)heap_caps_malloc(header->entry_count * sizeof(apu_log_entry_t), MALLOC_CAP_SPIRAM);
    if (!entries) {
        fprintf(stderr, "Error: Failed to allocate memory for entries\n");
        fclose(file);
        return NULL;
    }
    
    /* Read entries */
    size_t entries_read = fread(entries, sizeof(apu_log_entry_t), header->entry_count, file);
    if (entries_read != header->entry_count) {
        fprintf(stderr, "Error: Expected %u entries, read %zu\n", header->entry_count, entries_read);
        free(entries);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return entries;
}

int apuif_parse_apu_log(const char* filename) {
    apu_log_header_t header;
    apu_log_entry_t* entries = apuif_read_entries(filename, &header);
    if(!entries) return -1;

    /* Display entries */
    bool in_init = false;
    bool in_play = false;
    uint32_t current_frame = 0;
    
    printf("=== Log Entries ===\n");
    printf("   Index     Time  Addr  Data  Description\n");
    printf("-------- -------- ------ ---- -----------\n");
    
    for (uint32_t i = 0; i < header.entry_count; i++) {
        const apu_log_entry_t* entry = &entries[i];
        
        switch (entry->event_type) {
            case APU_EVENT_INIT_START:
                printf("\n>>> INIT START (Time %d) <<<\n", entry->time);
                in_init = true;
                break;
                
            case APU_EVENT_INIT_END:
                printf(">>> INIT END (Time %d) <<<\n\n", entry->time);
                in_init = false;
                break;
                
            case APU_EVENT_PLAY_START:
                printf("\n>>> PLAY START (Frame %u, Time %d) <<<\n", entry->frame_number, entry->time);
                in_play = true;
                current_frame = entry->frame_number;
                break;
                
            case APU_EVENT_PLAY_END:
                printf(">>> PLAY END (Frame %u, Time %d) <<<\n\n", entry->frame_number, entry->time);
                in_play = false;
                break;
                
            case APU_EVENT_WRITE:
            default:
                printf("%8u %8d 0x%04X 0x%02X %s", 
                       i + 1, entry->time, entry->addr, entry->data, get_register_name(entry->addr));
                if (in_init) printf(" [INIT]");
                else if (in_play) printf(" [PLAY Frame %u]", current_frame);
                printf("\n");
                break;
        }
    }
    
    /* Statistics */
    printf("\n=== Statistics ===\n");
    
    /* Count register writes */
    int reg_count[0x18] = {0};
    int write_count = 0;
    int32_t max_time = 0;
    
    for (uint32_t i = 0; i < header.entry_count; i++) {
        const apu_log_entry_t* entry = &entries[i];
        if (entry->event_type == APU_EVENT_WRITE) {
            write_count++;
            if (entry->addr >= 0x4000 && entry->addr <= 0x4017) {
                reg_count[entry->addr - 0x4000]++;
            }
            if (entry->time > max_time) {
                max_time = entry->time;
            }
        }
    }
    
    printf("Total register writes: %d\n", write_count);
    
    if (header.frame_count > 0) {
        double duration_sec = header.frame_count / 60.0; /* 60Hz NTSC */
        printf("Duration: %.3f seconds (%u frames @ 60Hz)\n", duration_sec, header.frame_count);
        printf("Average writes per frame: %.1f\n", (double)write_count / header.frame_count);
        printf("Average writes per second: %.1f\n", write_count / duration_sec);
    }
    
    printf("Max time value: %d CPU cycles\n", max_time);
    
    /* Register usage */
    printf("\n=== Register Usage ===\n");
    for (int i = 0; i < 0x18; i++) {
        if (reg_count[i] > 0) {
            printf("$%04X %s: %d writes\n", 0x4000 + i, get_register_name(0x4000 + i), reg_count[i]);
        }
    }
    
    /* Memory info */
    printf("\n=== Memory Info ===\n");
    printf("Header size: %zu bytes\n", sizeof(apu_log_header_t));
    printf("Entry size: %zu bytes\n", sizeof(apu_log_entry_t));
    printf("Total file size: %zu bytes\n", sizeof(apu_log_header_t) + header.entry_count * sizeof(apu_log_entry_t));
    
    free(entries);
    return true;
}

}

}
