#include "apu_if.h"
#include "nofrendo/noftypes.h"
#include "nes_apu.h"
#include "esp_heap_caps.h"

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

void apuif_init(){
    if(_initialized) return;

    _audio_frequency = 15720; //NTSC
    _audio_frame_samples = (_audio_frequency << 16)/60;   // fixed point sampler
    _audio_fraction = 0;

    _apu = apu_create(0, _audio_frequency, 60, 8);
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
