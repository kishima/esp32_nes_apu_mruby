#ifndef _APU_C_H_
#define _APU_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void apuif_init();
int apuif_frame_sample_count();
int apuif_process(int16_t* buff, int len);
void apuif_write_reg(uint32_t address, uint8_t value);
uint8_t apuif_read_reg(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif //_APU_C_H_

