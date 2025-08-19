#ifndef APU_STUB_H
#define APU_STUB_H

#include <stdint.h>

// APU register addresses
#define APU_PULSE1_VOL    0x4000
#define APU_PULSE1_SWEEP  0x4001
#define APU_PULSE1_LO     0x4002
#define APU_PULSE1_HI     0x4003
#define APU_PULSE2_VOL    0x4004
#define APU_PULSE2_SWEEP  0x4005
#define APU_PULSE2_LO     0x4006
#define APU_PULSE2_HI     0x4007
#define APU_TRIANGLE_LINEAR 0x4008
#define APU_TRIANGLE_LO   0x400A
#define APU_TRIANGLE_HI   0x400B
#define APU_NOISE_VOL     0x400C
#define APU_NOISE_LO      0x400E
#define APU_NOISE_HI      0x400F
#define APU_DMC_FREQ      0x4010
#define APU_DMC_RAW       0x4011
#define APU_DMC_START     0x4012
#define APU_DMC_LEN       0x4013
#define APU_STATUS        0x4015
#define APU_FRAME         0x4017

// APU stub functions
void apu_init(void);
void apu_write(uint16_t addr, uint8_t value);
uint8_t apu_read(uint16_t addr);
void apu_step(uint32_t cycles);

#endif // APU_STUB_H