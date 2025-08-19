#include <stdio.h>
#include "apu_stub.h"

// APU register names for debugging
static const char* get_apu_register_name(uint16_t addr) {
    switch(addr) {
        case 0x4000: return "PULSE1_VOL";
        case 0x4001: return "PULSE1_SWEEP";
        case 0x4002: return "PULSE1_LO";
        case 0x4003: return "PULSE1_HI";
        case 0x4004: return "PULSE2_VOL";
        case 0x4005: return "PULSE2_SWEEP";
        case 0x4006: return "PULSE2_LO";
        case 0x4007: return "PULSE2_HI";
        case 0x4008: return "TRIANGLE_LINEAR";
        case 0x400A: return "TRIANGLE_LO";
        case 0x400B: return "TRIANGLE_HI";
        case 0x400C: return "NOISE_VOL";
        case 0x400E: return "NOISE_LO";
        case 0x400F: return "NOISE_HI";
        case 0x4010: return "DMC_FREQ";
        case 0x4011: return "DMC_RAW";
        case 0x4012: return "DMC_START";
        case 0x4013: return "DMC_LEN";
        case 0x4015: return "APU_STATUS";
        case 0x4017: return "APU_FRAME";
        default: return "UNKNOWN";
    }
}

void apu_init(void) {
    printf("APU: Initialized (stub)\n");
}

void apu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x4000 && addr <= 0x4017) {
        printf("APU: Write $%04X (%s) = $%02X\n", addr, get_apu_register_name(addr), value);
        
        // Special handling for APU_STATUS register
        if (addr == 0x4015) {
            printf("  -> Channels enabled: ");
            if (value & 0x01) printf("PULSE1 ");
            if (value & 0x02) printf("PULSE2 ");
            if (value & 0x04) printf("TRIANGLE ");
            if (value & 0x08) printf("NOISE ");
            if (value & 0x10) printf("DMC ");
            printf("\n");
        }
    }
}

uint8_t apu_read(uint16_t addr) {
    if (addr == 0x4015) {
        // Return a dummy status value
        // In real implementation, this would return channel status
        printf("APU: Read $4015 (APU_STATUS)\n");
        return 0x00;
    }
    return 0x00;
}

void apu_step(uint32_t cycles) {
    // Stub: In real implementation, this would advance APU timing
}