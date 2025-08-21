#include "apu_if.h"
#include "nofrendo/noftypes.h"
#include "nes_apu.h"

extern "C" {

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

}
