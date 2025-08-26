#ifndef _APU_C_H_
#define _APU_C_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define USE_I2S

#ifdef USE_I2S
#define PIN_BCK   GPIO_NUM_26
#define PIN_WS    GPIO_NUM_25
#define PIN_DOUT  GPIO_NUM_33
#else
#define AUDIO_PIN   26
#endif

/* APU event types */
typedef enum {
    APU_EVENT_WRITE = 0,
    APU_EVENT_INIT_START,
    APU_EVENT_INIT_END,
    APU_EVENT_PLAY_START,
    APU_EVENT_PLAY_END
} apu_event_type_t;

/* Binary file format header */
typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t entry_count;
    uint32_t frame_count;
    uint32_t reserved[3];
} apu_log_header_t;

/* APU register write event */
typedef struct {
    int32_t time;
    uint16_t addr;
    uint8_t data;
    uint8_t event_type;
    uint32_t frame_number;
} apu_log_entry_t;

apu_log_entry_t* apuif_read_entries(const char* filename, apu_log_header_t* header);
int apuif_parse_apu_log(const char* filename);

void apuif_init();
int apuif_frame_sample_count();
int apuif_process(int16_t* buff, int len);
void apuif_write_reg(uint32_t address, uint8_t value);
uint8_t apuif_read_reg(uint32_t address);

void apuif_audio_write(const int16_t* s, int len, int channels);
int apuif_use_external_process();
void apuif_set_external_process(int flag);

#ifdef __cplusplus
}
#endif

#endif //_APU_C_H_

