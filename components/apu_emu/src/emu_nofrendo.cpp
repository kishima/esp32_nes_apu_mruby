
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

#include "emu.h"
// #include "media.h"  // Disable embedded ROM data
#include <stdint.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern "C" {
#include "nofrendo/osd.h"
#include "nofrendo/event.h"
#include "nofrendo/nes.h"
#include "nofrendo/nes_apu.h"
};
#include "math.h"

using namespace std;

// https://wiki.nesdev.com/w/index.php/NTSC_video
// NES/SMS have pixel rates of 5.3693175, or 2/3 color clock
// in 3 phase mode each pixel gets 2 DAC values written, 2 color clocks = 3 nes pixels

uint32_t nes_3_phase[64] = {
    0x2C2C2C00,0x241D2400,0x221D2600,0x1F1F2700,0x1D222600,0x1D242400,0x1D262200,0x1F271F00,
    0x22261D00,0x24241D00,0x26221D00,0x271F1F00,0x261D2200,0x14141400,0x14141400,0x14141400,
    0x38383800,0x2C252C00,0x2A252E00,0x27272F00,0x252A2E00,0x252C2C00,0x252E2A00,0x272F2700,
    0x2A2E2500,0x2C2C2500,0x2E2A2500,0x2F272700,0x2E252A00,0x1F1F1F00,0x15151500,0x15151500,
    0x45454500,0x3A323A00,0x37333C00,0x35353C00,0x33373C00,0x323A3A00,0x333C3700,0x353C3500,
    0x373C3300,0x3A3A3200,0x3C373300,0x3C353500,0x3C333700,0x2B2B2B00,0x16161600,0x16161600,
    0x45454500,0x423B4200,0x403B4400,0x3D3D4500,0x3B404400,0x3B424200,0x3B444000,0x3D453D00,
    0x40443B00,0x42423B00,0x44403B00,0x453D3D00,0x443B4000,0x39393900,0x17171700,0x17171700,
};
uint32_t nes_4_phase[64] = {
    0x2C2C2C2C,0x241D1F26,0x221D2227,0x1F1D2426,0x1D1F2624,0x1D222722,0x1D24261F,0x1F26241D,
    0x2227221D,0x24261F1D,0x26241D1F,0x27221D22,0x261F1D24,0x14141414,0x14141414,0x14141414,
    0x38383838,0x2C25272E,0x2A252A2F,0x27252C2E,0x25272E2C,0x252A2F2A,0x252C2E27,0x272E2C25,
    0x2A2F2A25,0x2C2E2725,0x2E2C2527,0x2F2A252A,0x2E27252C,0x1F1F1F1F,0x15151515,0x15151515,
    0x45454545,0x3A33353C,0x3732373C,0x35333A3C,0x33353C3A,0x32373C37,0x333A3C35,0x353C3A33,
    0x373C3732,0x3A3C3533,0x3C3A3335,0x3C373237,0x3C35333A,0x2B2B2B2B,0x16161616,0x16161616,
    0x45454545,0x423B3D44,0x403B4045,0x3D3B4244,0x3B3D4442,0x3B404540,0x3B42443D,0x3D44423B,
    0x4045403B,0x42443D3B,0x44423B3D,0x45403B40,0x443D3B42,0x39393939,0x17171717,0x17171717,
};


// PAL yuyv table, must be in RAM
uint32_t _nes_yuv_4_phase_pal[] = {
    0x31313131,0x2D21202B,0x2720252D,0x21212B2C,0x1D23302A,0x1B263127,0x1C293023,0x202B2D22,
    0x262B2722,0x2C2B2122,0x2F2B1E23,0x31291F27,0x30251F2A,0x18181818,0x19191919,0x19191919,
    0x3D3D3D3D,0x34292833,0x2F282D34,0x29283334,0x252B3732,0x232E392E,0x2431382B,0x28333429,
    0x2D342F28,0x33342928,0x3732252A,0x392E232E,0x382B2431,0x24242424,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x42373540,0x3C373B40,0x36374040,0x3337433F,0x3139433B,0x323D4338,0x35414237,
    0x3B423D35,0x41413736,0x453F3238,0x473C313B,0x4639323F,0x2F2F2F2F,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x48413D45,0x42404345,0x3D3F4644,0x3B3D4543,0x3B3E4542,0x3B42453F,0x3E47463E,
    0x434A453E,0x46483E3D,0x4843393E,0x4A403842,0x4B403944,0x3E3E3E3E,0x1B1B1B1B,0x1B1B1B1B,
    //odd
    0x31313131,0x20212D2B,0x2520272D,0x2B21212C,0x30231D2A,0x31261B27,0x30291C23,0x2D2B2022,
    0x272B2622,0x212B2C22,0x1E2B2F23,0x1F293127,0x1F25302A,0x18181818,0x19191919,0x19191919,
    0x3D3D3D3D,0x28293433,0x2D282F34,0x33282934,0x372B2532,0x392E232E,0x3831242B,0x34332829,
    0x2F342D28,0x29343328,0x2532372A,0x232E392E,0x242B3831,0x24242424,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x35374240,0x3B373C40,0x40373640,0x4337333F,0x4339313B,0x433D3238,0x42413537,
    0x3D423B35,0x37414136,0x323F4538,0x313C473B,0x3239463F,0x2F2F2F2F,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x3D414845,0x43404245,0x463F3D44,0x453D3B43,0x453E3B42,0x45423B3F,0x46473E3E,
    0x454A433E,0x3E48463D,0x3943483E,0x38404A42,0x39404B44,0x3E3E3E3E,0x1B1B1B1B,0x1B1B1B1B,
};

static void make_nes_pal_uv(uint8_t *u,uint8_t *v)
{
    for (int c = 0; c < 16; c++) {
        if (c == 0 || c > 12)
            ;
        else {
            float a = 2*M_PI*(c-1)/12 + 2*M_PI*(180-33)/360;    // just guessing at hue adjustment for PAL
            u[c] = cos(a)*127;
            v[c] = sin(a)*127;

            // get the phase offsets for even and odd pal lines
            //_p0[c] = atan2(0.436798*u[c],0.614777*v[c])*256/(2*M_PI) + 192;
            //_p1[c] = atan2(0.436798*u[c],-0.614777*v[c])*256/(2*M_PI) + 192;
        }
    }
}

// TODO. scale the u,v to fill range
uint32_t yuv_palette(int r, int g, int b)
{
    float y = 0.299 * r + 0.587 *g + 0.114 * b;
    float u = -0.147407 * r - 0.289391 * g + 0.436798 * b;
    float v =  0.614777 * r - 0.514799 * g - 0.099978 * b;
    int luma = y/255*(WHITE_LEVEL-BLACK_LEVEL) + BLACK_LEVEL;
    uint8_t ui = u;
    uint8_t vi = v;
    return ((luma & 0xFF00) << 16) | ((ui & 0xF8) << 8) | (vi >> 3); // luma:0:u:v
}

// copy from emu_atari800.cpp
// make_yuv_palette from RGB palette
void make_yuv_palette(const char* name, const uint32_t* rgb, int len)
{
    uint32_t pal[256*2];
    uint32_t* even = pal;
    uint32_t* odd = pal + len;

    float chroma_scale = BLANKING_LEVEL/2/256;
    //chroma_scale /= 127;  // looks a little washed out
    chroma_scale /= 80;
    for (int i = 0; i < len; i++) {
        uint8_t r = rgb[i] >> 16;
        uint8_t g = rgb[i] >> 8;
        uint8_t b = rgb[i];

        float y = 0.299 * r + 0.587*g + 0.114 * b;
        float u = -0.147407 * r - 0.289391 * g + 0.436798 * b;
        float v =  0.614777 * r - 0.514799 * g - 0.099978 * b;
        y /= 255.0;
        y = (y*(WHITE_LEVEL-BLACK_LEVEL) + BLACK_LEVEL)/256;

        uint32_t e = 0;
        uint32_t o = 0;
        for (int i = 0; i < 4; i++) {
            float p = 2*M_PI*i/4 + M_PI;
            float s = sin(p)*chroma_scale;
            float c = cos(p)*chroma_scale;
            uint8_t e0 = round(y + (s*u) + (c*v));
            uint8_t o0 = round(y + (s*u) - (c*v));
            e = (e << 8) | e0;
            o = (o << 8) | o0;
        }
        *even++ = e;
        *odd++ = o;
    }

    printf("uint32_t %s_4_phase_pal[] = {\n",name);
    for (int i = 0; i < len*2; i++) {  // start with luminance map
        printf("0x%08X,",pal[i]);
        if ((i & 7) == 7)
            printf("\n");
        if (i == (len-1)) {
            printf("//odd\n");
        }
    }
    printf("};\n");

    /*
     // don't bother with phase tables
    printf("uint8_t DRAM_ATTR %s[] = {\n",name);
    for (int i = 0; i < len*(1<<PHASE_BITS)*2; i++) {
        printf("0x%02X,",yuv[i]);
        if ((i & 15) == 15)
            printf("\n");
        if (i == (len*(1<<PHASE_BITS)-1)) {
            printf("//odd\n");
        }
    }
    printf("};\n");
     */
}

extern rgb_t nes_palette[64];
extern "C" void pal_generate();
void make_alt_pal()
{
    pal_generate();
    uint32_t pal[64];
    for (int i = 0; i < 64; i++) {
        auto p = nes_palette[i];
        pal[i] = (p.r << 16) | (p.g << 8) | p.b;
    }
    make_yuv_palette("_nes_yuv",pal,64);
}

static const float _nes_luma[] = {
    0.50,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.29,0.00,0.02,0.02,
    0.75,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.45,0.24,0.04,0.04,
    1.00,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.73,0.47,0.05,0.05,
    1.00,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.77,0.07,0.07,
};

static void make_nes_palette(int phases)
{
    float saturation = 0.5;
    printf("uint32_t nes_%d_phase[64] = {\n",phases);
    for (int i = 0; i < 64; i++) {
        int chroma = (i & 0xF);
        int luma = _nes_luma[i]*(WHITE_LEVEL-BLACK_LEVEL) + BLANKING_LEVEL;

        int p[8] = {0};
        for (int j = 0; j < phases; j++)
            p[j] = luma;  // 0x1D is really black, really BLANKING_LEVEL

        chroma -= 1;
        if (chroma >= 0 && chroma < 12) {
            for (int j = 0; j < phases; j++)
                p[j] += sin(2*M_PI*(5 + chroma + (12/phases)*j)/12) * BLANKING_LEVEL/2*saturation;   // not sure why 5 is the right offset
        }

        uint32_t pi = 0;
        for (int j = 0; j < 4; j++)
            pi = (pi << 8) | p[j] >> 8;
        printf("0x%08X,",pi);
        if ((i & 7) == 7)
            printf("\n");
    }
    printf("};\n");
}

uint8_t* _nofrendo_rom = 0;
extern "C"
char *osd_getromdata()
{
    return (char *)_nofrendo_rom;
}

extern "C"
int nes_emulate_init(const char* path, int width, int height);

extern "C"
uint8_t** nes_emulate_frame(bool draw_flag);

extern "C"
void nes_renderframe_audio();
//uint8_t** nes_emulate_frame_audio(bool draw_flag);

// NSF mapper support
extern "C" {
#include "nofrendo/nes_mmc.h"
#include "nofrendo/nes_rom.h"
#include "nofrendo/nes6502.h"

// Function declarations
int nes6502_execute(int cycles);
void nes6502_getcontext(nes6502_context *context);
void nes6502_setcontext(nes6502_context *context);
}

static void (*nes_sound_cb)(void *buffer, int length) = 0;

extern uint32_t nes_pal[256];

const char* _nes_help[] = {
    "Keyboard:",
    "  Arrow Keys - D-Pad",
    "  Left Shift - Button A",
    "  Option     - Button B",
    "  Return     - Start",
    "  Tab        - Select",
    "",
    "Wiimote (held sideways):",
    "  +          - Start",
    "  -          - Select",
    "  + & -      - Reset",
    "  A,1        - Button A",
    "  B,2        - Button B",
    0
};

const char* _nes_ext[] = {
    "nes",
    0
};

int _audio_frequency;
extern "C"
void osd_getsoundinfo(sndinfo_t *info)
{
    info->sample_rate = _audio_frequency;
    info->bps = 8;
}

extern "C"
void osd_setsound(void (*playfunc)(void *buffer, int length))
{
    nes_sound_cb = playfunc;
}

std::string to_string(int i);
class EmuNofrendo : public Emu {
    uint8_t** _lines;
public:
    EmuNofrendo(int ntsc) : Emu("nofrendo",256,240,ntsc,(16 | (1 << 8)),4,EMU_NES)    // audio is 16bit, 3 or 6 cc width
    {
        _lines = 0;
        _ext = _nes_ext;
        _help = _nes_help;
        _audio_frequency = audio_frequency;
    }

    virtual void gen_palettes()
    {
        make_nes_palette(3);
        make_nes_palette(4);
        make_alt_pal();
    }

    virtual int info(const string& file, vector<string>& strs)
    {
        string ext = get_ext(file);
        uint8_t hdr[15];
        int len = Emu::head(file,hdr,sizeof(hdr));
        string name = file.substr(file.find_last_of("/") + 1);
        strs.push_back(name);
        strs.push_back(::to_string(len/1024) + "k NES Cartridge");
        strs.push_back("");
        if (hdr[0] == 'N' && hdr[1] == 'E' && hdr[2] == 'S') {
            int prg = hdr[4] * 16;
            int chr = hdr[5] * 8;
            int mapper = (hdr[6] >> 4) | (hdr[7] & 0xF0);
            char buf[64];
            sprintf(buf,"MAP:%d",mapper);
            strs.push_back(buf);
            sprintf(buf,"PRG:%dk",prg);
            strs.push_back(buf);
            sprintf(buf,"CHR:%dk",chr);
            strs.push_back(buf);
        }
        return 0;
    }

    void pad(int pressed, int index)
    {
        event_t e = event_get(index);
        e(pressed);
    }

    enum {
        event_joypad1_up_ = 1,
        event_joypad1_down_ = 2,
        event_joypad1_left_ = 4,
        event_joypad1_right_ = 8,
        event_joypad1_start_ = 16,
        event_joypad1_select_ = 32,
        event_joypad1_a_ = 64,
        event_joypad1_b_ = 128,

        event_soft_reset_ = 256,
        event_hard_reset_ = 512
    };

    const int _nes_1[11] = {
        event_joypad1_up,
        event_joypad1_down,
        event_joypad1_left,
        event_joypad1_right,
        event_joypad1_start,
        event_joypad1_select,
        event_joypad1_a,
        event_joypad1_b,

        event_soft_reset,
        event_hard_reset,
        0
    };

    const int _nes_2[9] = {
        event_joypad2_up,
        event_joypad2_down,
        event_joypad2_left,
        event_joypad2_right,
        event_joypad2_start,
        event_joypad2_select,
        event_joypad2_a,
        event_joypad2_b,
        0
    };

    // Rotated 90%
    const uint32_t _common_nes[16] = {
        0,  // msb
        0,
        0,
        event_joypad1_start_,       // PLUS
        event_joypad1_left_,        // UP
        event_joypad1_right_,       // DOWN
        event_joypad1_up_,          // RIGHT
        event_joypad1_down_,        // LEFT

        0, // HOME
        0,
        0,
        event_joypad1_select_,  // MINUS
        event_joypad1_a_,      // A
        event_joypad1_b_,      // B
        event_joypad1_b_,      // ONE
        event_joypad1_a_,      // TWO
    };

    const uint32_t _classic_nes[16] = {
        event_joypad1_right_,    // RIGHT
        event_joypad1_down_,     // DOWN
        0,                       // LEFT_TOP
        event_joypad1_select_,    // MINUS
        0,                        // HOME
        event_joypad1_start_,     // PLUS
        0,                    // RIGHT_TOP
        0,

        0,                  // LOWER_LEFT
        event_joypad1_b_,   // B
        0,                  // Y
        event_joypad1_a_,   // A
        0,                  // X
        0,                  // LOWER_RIGHT
        event_joypad1_left_, // LEFT
        event_joypad1_up_   // UP
    };

    const uint32_t _generic_nes[16] = {
        0,                  // GENERIC_OTHER   0x8000
        0,                  // GENERIC_FIRE_X  0x4000  // RETCON
        0,                  // GENERIC_FIRE_Y  0x2000
        0,                  // GENERIC_FIRE_Z  0x1000

        event_joypad1_a_,      //GENERIC_FIRE_A  0x0800
        event_joypad1_b_,      //GENERIC_FIRE_B  0x0400
        0,                      //GENERIC_FIRE_C  0x0200
        0,                      //GENERIC_RESET   0x0100     // ATARI FLASHBACK

        event_joypad1_start_,   //GENERIC_START   0x0080
        event_joypad1_select_,  //GENERIC_SELECT  0x0040
        event_joypad1_a_,      //GENERIC_FIRE    0x0020
        event_joypad1_right_,  //GENERIC_RIGHT   0x0010

        event_joypad1_left_,   //GENERIC_LEFT    0x0008
        event_joypad1_down_,   //GENERIC_DOWN    0x0004
        event_joypad1_up_,      //GENERIC_UP      0x0002
        0,                      //GENERIC_MENU    0x0001
    };

    // raw HID data. handle WII/IR mappings
    virtual void hid(const uint8_t* d, int len)
    {
        return;
        #if 0 //disable HID
        if (d[0] != 0x32 && d[0] != 0x42)
            return;
        bool ir = *d++ == 0x42;

        for (int i = 0; i < 2; i++) {
            uint32_t p;
            if (ir) {
                int m = d[0] + (d[1] << 8);
                p = generic_map(m,_generic_nes);
                d += 2;
            } else
                p = wii_map(i,_common_nes,_classic_nes);

            // reset on select + start held at the same time
            if ((p & event_joypad1_select_) && (p & event_joypad1_start_))
                pad(1,event_soft_reset);

            const int* m = i ? _nes_2 : _nes_1;
            for (int e = 0; m[e]; e++)
            {
                pad((p & 1),m[e]);
                p >>= 1;
            }
        }
        #endif
    }

    /*
     Return - Joypad 1 Start
     Tab - Joypad 1 Select
     */
    virtual void key(int keycode, int pressed, int mods)
    {
        switch (keycode) {
            case 82: pad(pressed,event_joypad1_up); break;
            case 81: pad(pressed,event_joypad1_down); break;
            case 80: pad(pressed,event_joypad1_left); break;
            case 79: pad(pressed,event_joypad1_right); break;

            case 21: pad(pressed,event_soft_reset); break; // soft reset - 'r'
            case 23: pad(pressed,event_hard_reset); break; // hard reset - 't'

            case 61: pad(pressed,event_joypad1_start); break; // F4
            case 62: pad(pressed,((KEY_MOD_LSHIFT|KEY_MOD_RSHIFT) & mods) ? event_hard_reset : event_soft_reset); break; // F5

            case 40: pad(pressed,event_joypad1_start); break; // return
            case 43: pad(pressed,event_joypad1_select); break; // tab

            case 225: pad(pressed,event_joypad1_a); break; // left shift key event_joypad1_a
            case 226: pad(pressed,event_joypad1_b); break; // option key event_joypad1_b

            //case 59: system(pressed,INPUT_PAUSE); break; // F2
            //case 61: system(pressed,INPUT_START); break; // F4
            //case 62: system(pressed,((KEY_MOD_LSHIFT|KEY_MOD_RSHIFT) & mods) ? INPUT_HARD_RESET : INPUT_SOFT_RESET); break; // F5
        }
    }

    // Load ROM data from SPIFFS
    uint8_t* load_rom_from_spiffs(const char* filename, int* size) {
        FILE* file = fopen(filename, "rb");
        if (!file) {
            printf("Failed to open ROM file: %s\n", filename);
            return nullptr;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate memory and read file
        uint8_t* rom_data = (uint8_t*)malloc(*size);
        if (!rom_data) {
            printf("Failed to allocate memory for ROM\n");
            fclose(file);
            return nullptr;
        }

        size_t read_bytes = fread(rom_data, 1, *size, file);
        fclose(file);

        if (read_bytes != *size) {
            printf("Failed to read complete ROM file\n");
            free(rom_data);
            return nullptr;
        }

        printf("Loaded ROM %s: %d bytes\n", filename, *size);
        return rom_data;
    }

    virtual int insert(const std::string& path, int flags, int disk_index)
    {
        // Free previous ROM data
        if (_nofrendo_rom) {
            free(_nofrendo_rom);
            _nofrendo_rom = nullptr;
        }
        
        printf("nofrendo inserting ROM from SPIFFS: %s\n", path.c_str());

        // Load ROM from SPIFFS
        int rom_size;
        _nofrendo_rom = load_rom_from_spiffs(path.c_str(), &rom_size);
        if (!_nofrendo_rom) {
            printf("nofrendo can't load ROM from SPIFFS: %s\n", path.c_str());
            return -1;
        }

        printf("nofrendo loaded ROM %s: %d bytes\n", path.c_str(), rom_size);

        // Initialize NES emulation with ROM data
        nes_emulate_init(path.c_str(), width, height);
        _lines = nes_emulate_frame(true);   // first frame!
        return 0;
    }

    virtual int update()
    {
        if (_nofrendo_rom)
            _lines = nes_emulate_frame(true);
        return 0;
    }

    virtual uint8_t** video_buffer()
    {
        return _lines;
    }
    
    virtual int audio_buffer(int16_t* b, int len)
    {
        int n = frame_sample_count();
        if (nes_sound_cb) {
            nes_sound_cb(b,n);  // 8 bit unsigned
            uint8_t* b8 = (uint8_t*)b;
            for (int i = n-1; i >= 0; i--)
                b[i] = (b8[i] ^ 0x80) << 8;  // turn it back into signed 16
        }
        else
            memset(b,0,2*n);
        return n;
    }

    virtual const uint32_t* ntsc_palette() { return cc_width == 3 ? nes_3_phase : nes_4_phase; };
    virtual const uint32_t* pal_palette() { return _nes_yuv_4_phase_pal; };
    virtual const uint32_t* rgb_palette() { return nes_pal; };
};

Emu* NewNofrendo(int ntsc)
{
    return new EmuNofrendo(ntsc);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++;

// NSF file header structure
struct NSFHeader {
    char signature[5];        // "NESM" + 0x1A
    uint8_t version;         // Version number
    uint8_t total_songs;     // Total number of songs
    uint8_t starting_song;   // Starting song (1-based)
    uint16_t load_addr;      // Load address (little endian)
    uint16_t init_addr;      // Init address (little endian)
    uint16_t play_addr;      // Play address (little endian)
    char song_name[32];      // Song name (null terminated)
    char artist[32];         // Artist name (null terminated)
    char copyright[32];      // Copyright (null terminated)
    uint16_t ntsc_speed;     // NTSC speed (little endian)
    uint8_t bankswitch[8];   // Bank switching info
    uint16_t pal_speed;      // PAL speed (little endian)
    uint8_t pal_ntsc_flags;  // PAL/NTSC flags
    uint8_t extra_sound;     // Extra sound chip support
    uint8_t reserved[4];     // Reserved bytes
};

class EmuNsfPlay : public Emu {
    uint8_t** _lines;
    NSFHeader _nsf_header;
    uint16_t _current_song;
    bool _nsf_initialized;
    bool _play_setup_done;
    
    // APUメモリページの手動設定
    void nsf_setup_apu_memory_page() {
        printf("NSF: Setting up APU memory page at $4000-$40FF\n");
        
        // APUレジスタ用のメモリ領域を確保
        static uint8_t apu_memory_page[256];
        memset(apu_memory_page, 0, sizeof(apu_memory_page));
        
        // CPU contextのメモリページテーブルに直接設定
        nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
        if (cpu_ctx) {
            cpu_ctx->mem_page[0x40] = apu_memory_page;
            printf("NSF: APU memory page set at %p\n", apu_memory_page);
        } else {
            printf("NSF: Failed to get CPU context for memory page setup\n");
        }
    }
    
    // NSF ROMメモリページの手動設定
    void nsf_setup_rom_memory_page() {
        printf("NSF: Setting up NSF ROM memory mapping\n");
        
        if (!_nofrendo_rom) {
            printf("NSF: No NSF ROM data available\n");
            return;
        }
        
        // NSFデータのヘッダをスキップして、実際の音楽データを取得
        uint8_t* nsf_data = _nofrendo_rom + sizeof(NSFHeader);
        
        // CPU contextのメモリページテーブルにNSFデータをマップ
        nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
        if (cpu_ctx) {
            // $8000-$8FFF page (0x80)
            cpu_ctx->mem_page[0x80] = nsf_data;
            // $9000-$9FFF page (0x90) 
            cpu_ctx->mem_page[0x90] = nsf_data + 0x1000;
            // $A000-$AFFF page (0xA0)
            cpu_ctx->mem_page[0xA0] = nsf_data + 0x2000;
            // $B000-$BFFF page (0xB0)  
            cpu_ctx->mem_page[0xB0] = nsf_data + 0x3000;
            // $C000-$CFFF page (0xC0)
            cpu_ctx->mem_page[0xC0] = nsf_data + 0x4000;
            // $D000-$DFFF page (0xD0)
            cpu_ctx->mem_page[0xD0] = nsf_data + 0x5000;
            // $E000-$EFFF page (0xE0)
            cpu_ctx->mem_page[0xE0] = nsf_data + 0x6000;
            // $F000-$FFFF page (0xF0)
            cpu_ctx->mem_page[0xF0] = nsf_data + 0x7000;
            
            printf("NSF: ROM mapped from $8000-$FFFF at %p\n", nsf_data);
        } else {
            printf("NSF: Failed to get CPU context for ROM mapping\n");
        }
    }
    
    // NSF専用APUハンドラ設定
    bool nsf_setup_apu_handlers() {
        printf("NSF: Setting up APU memory handlers for $4000-$4015\n");
        
        // Get NES context
        nes_t* nes = nes_getcontextptr();
        if (!nes || !nes->cpu) {
            printf("NSF: No NES context available for APU setup\n");
            return false;
        }
        
        // Debug current memory handler status
        printf("NSF: Checking current memory handlers...\n");
        bool apu_handler_found = false;
        for (int i = 0; i < 16 && nes->readhandler[i].read_func; i++) {
            printf("NSF: Read handler %d: $%04X-$%04X\n", i, 
                   nes->readhandler[i].min_range, nes->readhandler[i].max_range);
            if (nes->readhandler[i].min_range == 0x4000 && 
                nes->readhandler[i].max_range == 0x4015) {
                apu_handler_found = true;
            }
        }
        
        if (apu_handler_found) {
            printf("NSF: APU handlers already set up\n");
            return true;
        }
        
        // Add APU handlers manually if not found
        printf("NSF: Adding APU handlers manually\n");
        
        // Find empty handler slot
        int slot = -1;
        for (int i = 0; i < 16; i++) {
            if (!nes->readhandler[i].read_func) {
                slot = i;
                break;
            }
        }
        
        if (slot == -1) {
            printf("NSF: No available memory handler slots\n");
            return false;
        }
        
        // Setup APU read/write handlers
        nes->readhandler[slot].read_func = apu_read;
        nes->readhandler[slot].min_range = 0x4000;
        nes->readhandler[slot].max_range = 0x4015;
        nes->writehandler[slot].write_func = apu_write;
        nes->writehandler[slot].min_range = 0x4000;
        nes->writehandler[slot].max_range = 0x4015;
        
        printf("NSF: APU handlers configured in slot %d\n", slot);
        return true;
    }
public:
    EmuNsfPlay(int ntsc) : Emu("nofrendo",256,240,ntsc,(16 | (1 << 8)),4,EMU_NES)    // audio is 16bit, 3 or 6 cc width
    {
        _lines = 0;
        _audio_frequency = audio_frequency;
        _current_song = 1;
        _nsf_initialized = false;
        _play_setup_done = false;
        memset(&_nsf_header, 0, sizeof(_nsf_header));
    }

    // Parse NSF header
    bool parse_nsf_header(uint8_t* nsf_data, int size) {
        if (size < sizeof(NSFHeader)) {
            printf("NSF file too small\n");
            return false;
        }

        memcpy(&_nsf_header, nsf_data, sizeof(NSFHeader));

        // Verify NSF signature
        if (strncmp(_nsf_header.signature, "NESM", 4) != 0 || _nsf_header.signature[4] != 0x1A) {
            printf("Invalid NSF signature\n");
            return false;
        }

        printf("NSF Header Info:\n");
        printf("  Version: %d\n", _nsf_header.version);
        printf("  Total songs: %d\n", _nsf_header.total_songs);
        printf("  Starting song: %d\n", _nsf_header.starting_song);
        printf("  Load address: $%04X\n", _nsf_header.load_addr);
        printf("  Init address: $%04X\n", _nsf_header.init_addr);
        printf("  Play address: $%04X\n", _nsf_header.play_addr);
        printf("  Song name: %.32s\n", _nsf_header.song_name);
        printf("  Artist: %.32s\n", _nsf_header.artist);
        printf("  Copyright: %.32s\n", _nsf_header.copyright);

        _current_song = _nsf_header.starting_song;
        return true;
    }

    // Set up CPU for NSF PLAY routine execution
    void nsf_setup_play() {
        if (!_nsf_initialized) {
            printf("NSF: Not initialized, cannot setup PLAY\n");
            return;
        }
        
        // Get direct CPU context access
        nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
        if (!cpu_ctx) {
            printf("NSF: No CPU context for PLAY setup\n");
            return;
        }
        
        // Check if memory pages are properly initialized
        if (!cpu_ctx->mem_page[0]) {
            printf("NSF: CPU memory not initialized, skipping PLAY setup\n");
            return;
        }
        
        // Set up CPU for PLAY routine - 毎回新しい状態で始める
        cpu_ctx->pc_reg = _nsf_header.play_addr;  // PLAY routine address
        cpu_ctx->s_reg = 0xFF;  // スタックポインタをリセット
        cpu_ctx->p_reg = 0x04 | 0x02 | 0x20;     // I_FLAG | Z_FLAG | R_FLAG
        
        // Create safe dummy return area with NOP loop (一度だけ設定)
        uint8_t *zp = cpu_ctx->mem_page[0];  // Zero page $0000-$00FF
        if (zp) {
            // Create NOP loop at $00F0: NOP; JMP $00F0
            zp[0xF0] = 0xEA;  // NOP instruction
            zp[0xF1] = 0x4C;  // JMP absolute instruction
            zp[0xF2] = 0xF0;  // Jump target low byte ($00F0)
            zp[0xF3] = 0x00;  // Jump target high byte
            
            // Push dummy return address onto stack - 毎回新しいスタックで設定
            uint8_t *stack = cpu_ctx->mem_page[0] + 0x100;
            stack[0xFF] = 0x00;  // Return to $00F0 (NOP loop) high byte
            stack[0xFE] = 0xEF;  // Return to $00EF (before NOP loop) low byte
            cpu_ctx->s_reg = 0xFD;  // スタックポインタを設定
        }
        
        //printf("NSF: CPU setup for PLAY at $%04X, SP=$%02X\n", _nsf_header.play_addr, cpu_ctx->s_reg);
    }

    // Execute NSF INIT routine safely without full NES frame rendering
    void nsf_execute_init_routine() {
        printf("NSF: Executing INIT routine at $%04X\n", _nsf_header.init_addr);
        
        // メモリページを事前に設定
        printf("NSF: Pre-setting up memory pages before context access...\n");
        nsf_setup_apu_memory_page();
        nsf_setup_rom_memory_page();
        
        // CPU contextを直接取得して状態確認（nes6502_getcontextを回避）
        nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
        if (!cpu_ctx) {
            printf("ERROR: No CPU context available\n");
            return;
        }
        
        printf("DEBUG: INIT start PC=$%04X SP=$%02X\n", cpu_ctx->pc_reg, cpu_ctx->s_reg);
        
        // メモリページの状態を確認
        printf("DEBUG: Memory pages: page[0]=%p, page[0x40]=%p\n", 
               cpu_ctx->mem_page[0], cpu_ctx->mem_page[0x40]);
        
        if (cpu_ctx->mem_page[0x80]) {
            uint8_t *rom = cpu_ctx->mem_page[0x80];
            printf("DEBUG: ROM at $8000: %02X %02X %02X %02X\n", 
                   rom[0], rom[1], rom[2], rom[3]);
        } else {
            printf("ERROR: ROM still not mapped at $8000 after setup\n");
            return;
        }
        
        if (!cpu_ctx->mem_page[0x40]) {
            printf("ERROR: APU page still not set after setup\n");
            return;
        }
        
        //int start_time = esp_log_timestamp();
        
        // シンプルな実行で問題を特定
        printf("NSF: Executing INIT with simple method\n");
        int executed_total = 0;
        
        // 非常に短いサイクル数で実行してテスト
        for (int i = 0; i < 10; i++) {            
            int executed = nes6502_execute(100);  // 100サイクルずつ実行
            executed_total += executed;
            
            printf("NSF: INIT chunk %d: executed %d cycles\n", i, executed);
            
            // 実行サイクル数が要求より少ない場合は終了
            if (executed < 100) {
                printf("NSF: INIT routine completed early after %d total cycles\n", executed_total);
                break;
            }
        }
        
        //int end_time = esp_log_timestamp();
        // 
        // デバッグ：実行後のCPU状態を確認
        // if (cpu_ctx) {
        //     printf("DEBUG: INIT end PC=$%04X SP=$%02X, executed=%d cycles, time=%dms\n", 
        //            cpu_ctx->pc_reg, cpu_ctx->s_reg, executed_total, end_time - start_time);
        // }
               
        printf("NSF: INIT routine executed (%d total cycles)\n", executed_total);
    }

    // Execute NSF PLAY routine safely without full NES frame rendering
    void nsf_execute_play_routine() {
        if (!_nsf_initialized) return;
                // 毎回PLAYルーチンのためにCPU状態を再設定
        nsf_setup_play();
        
        // デバッグログを減らしてパフォーマンスを向上
        static uint32_t play_count = 0;
        bool show_debug = (play_count % 60 == 0);  // 1秒ごとに表示
        show_debug = true;  // 一時的に常に表示
        
        if (show_debug) {
            nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
            if (cpu_ctx) {
                printf("DEBUG: PLAY[%lu] start PC=$%04X SP=$%02X\n", play_count, cpu_ctx->pc_reg, cpu_ctx->s_reg);
            }
        }
        
        // 短いサイクル数でPLAYルーチンを実行
        int play_cycles = 100;  // さらに短くして安全性を向上
        //int start_time = esp_log_timestamp();
        
        int executed = nes6502_execute(play_cycles);
        
        //int end_time = esp_log_timestamp();
        play_count++;
        
        if (show_debug) {
            nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
            if (cpu_ctx) {
                // printf("DEBUG: PLAY[%lu] end PC=$%04X SP=$%02X, executed=%d cycles, time=%dms\n", 
                //        play_count-1, cpu_ctx->pc_reg, cpu_ctx->s_reg, executed, end_time - start_time);
                
                // APUレジスターの状態をチェック（安全なアクセス）
                if (cpu_ctx->mem_page[0x40] && 
                    (uintptr_t)cpu_ctx->mem_page[0x40] >= 0x3f000000 && 
                    (uintptr_t)cpu_ctx->mem_page[0x40] < 0x40000000) {
                    uint8_t *apu_regs = cpu_ctx->mem_page[0x40];
                    printf("APU: $4000=%02X $4001=%02X $4002=%02X $4003=%02X\n", 
                           apu_regs[0x00], apu_regs[0x01], apu_regs[0x02], apu_regs[0x03]);
                    printf("APU: $4004=%02X $4005=%02X $4006=%02X $4007=%02X\n", 
                           apu_regs[0x04], apu_regs[0x05], apu_regs[0x06], apu_regs[0x07]);
                    printf("APU: $4015=%02X (channel enable)\n", apu_regs[0x15]);
                } else {
                    printf("APU: Register access not available (invalid mapping)\n");
                }
            }
        }
        
        // 実行時間が異常に長い場合は警告
        // if (end_time - start_time > 5) {
        //     printf("WARNING: PLAY routine took %dms - too long!\n", end_time - start_time);
        // }
    }

    // Initialize NSF playback for specific song
    bool nsf_init_song(uint16_t song_number) {
        if (song_number < 1 || song_number > _nsf_header.total_songs) {
            printf("NSF: Invalid song number %d (valid: 1-%d)\n", 
                   song_number, _nsf_header.total_songs);
            return false;
        }

        _current_song = song_number;
        
        printf("NSF: Initializing song %d/%d\n", song_number, _nsf_header.total_songs);
        printf("NSF: INIT address: $%04X\n", _nsf_header.init_addr);
        printf("NSF: PLAY address: $%04X\n", _nsf_header.play_addr);

        // Set up 6502 CPU state for NSF INIT call using direct context access
        nes6502_context* cpu_ctx = nes_getcontextptr()->cpu;
        if (!cpu_ctx) {
            printf("NSF: No CPU context available for INIT setup\n");
            return false;
        }
        
        // Check if memory pages are properly initialized
        if (!cpu_ctx->mem_page[0]) {
            printf("NSF: CPU memory not initialized, cannot setup INIT\n");
            return false;
        }
        
        // Set NSF-specific CPU registers
        cpu_ctx->pc_reg = _nsf_header.init_addr;  // INIT routine address
        cpu_ctx->a_reg = song_number - 1;         // Song number (0-based)
        cpu_ctx->x_reg = 0;                       // 0 = NTSC, 1 = PAL
        cpu_ctx->s_reg = 0xFF;                    // Initialize stack pointer
        cpu_ctx->p_reg = 0x04 | 0x02 | 0x20;     // I_FLAG | Z_FLAG | R_FLAG
        
        printf("NSF: CPU INIT setup - PC=$%04X A=$%02X X=$%02X\n", 
               cpu_ctx->pc_reg, cpu_ctx->a_reg, cpu_ctx->x_reg);
        
        // Create safe dummy return area with NOP loop for INIT
        uint8_t *zp = cpu_ctx->mem_page[0];  // Zero page $0000-$00FF
        if (zp) {
            // Create NOP loop at $00F0: NOP; JMP $00F0 (same as PLAY)
            zp[0xF0] = 0xEA;  // NOP instruction
            zp[0xF1] = 0x4C;  // JMP absolute instruction  
            zp[0xF2] = 0xF0;  // Jump target low byte ($00F0)
            zp[0xF3] = 0x00;  // Jump target high byte
            
            // Push dummy return address onto stack for INIT RTS
            uint8_t *stack = cpu_ctx->mem_page[0] + 0x100;
            stack[0xFF] = 0x00;  // Return to $00F0 (NOP loop)
            stack[0xFE] = 0xF0;
            // Adjust stack pointer to account for pushed addresses
            cpu_ctx->s_reg = 0xFD;
        }
        
        _nsf_initialized = true;
        return true;
    }

    // Change current song (runtime song switching)
    void nsf_change_song(uint16_t song_number) {
        _play_setup_done = false;  // Reset for new song
        if (nsf_init_song(song_number)) {
            printf("NSF: Changed to song %d\n", song_number);
        }
    }

    // Detect NSF mapper type from bankswitch info
    int detect_nsf_mapper() {
        // Check if bankswitch is used
        bool has_bankswitch = false;
        for (int i = 0; i < 8; i++) {
            if (_nsf_header.bankswitch[i] != 0) {
                has_bankswitch = true;
                break;
            }
        }

        if (!has_bankswitch) {
            printf("NSF: No bankswitch detected, using Mapper 0 (NROM)\n");
            return 0;  // NROM - no mapper
        }

        // Most NSF files with bankswitch use MMC1 or MMC3-style mapping
        // Default to MMC1 (Mapper 1) for compatibility
        printf("NSF: Bankswitch detected, using Mapper 1 (MMC1)\n");
        printf("NSF: Bankswitch pattern: ");
        for (int i = 0; i < 8; i++) {
            printf("%02X ", _nsf_header.bankswitch[i]);
        }
        printf("\n");
        
        return 1;  // MMC1
    }

    // Setup NSF mapper based on detected type
    bool setup_nsf_mapper(int mapper_number) {
        // Create a dummy rominfo structure for NSF
        rominfo_t nsf_rominfo;
        memset(&nsf_rominfo, 0, sizeof(nsf_rominfo));
        
        // Set up basic ROM info for NSF
        nsf_rominfo.mapper_number = mapper_number;
        nsf_rominfo.rom_banks = 32;      // Assume 512KB ROM space for NSF
        nsf_rominfo.vrom_banks = 0;      // NSF usually has no CHR ROM
        nsf_rominfo.mirror = MIRROR_VERT; // Default mirroring
        nsf_rominfo.flags = 0;
        
        // Create NSF filename
        strcpy(nsf_rominfo.filename, "nsf_player.nsf");
        
        printf("NSF: Setting up mapper %d for NSF playback\n", mapper_number);
        
        // Create mapper context
        mmc_t *nsf_mmc = mmc_create(&nsf_rominfo);
        if (!nsf_mmc) {
            printf("NSF: Failed to create mapper %d\n", mapper_number);
            return false;
        }
        
        printf("NSF: Mapper %d (%s) initialized successfully\n", 
               mapper_number, nsf_mmc->intf->name);
        
        return true;
    }

    // Load NSF data from SPIFFS
    uint8_t* load_nsf_from_spiffs(const char* filename, int* size) {
        FILE* file = fopen(filename, "rb");
        if (!file) {
            printf("Failed to open ROM file: %s\n", filename);
            return nullptr;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate memory and read file
        uint8_t* rom_data = (uint8_t*)malloc(*size);
        if (!rom_data) {
            printf("Failed to allocate memory for ROM\n");
            fclose(file);
            return nullptr;
        }

        size_t read_bytes = fread(rom_data, 1, *size, file);
        fclose(file);

        if (read_bytes != *size) {
            printf("Failed to read complete ROM file\n");
            free(rom_data);
            return nullptr;
        }

        printf("Loaded ROM %s: %d bytes\n", filename, *size);
        return rom_data;
    }

    virtual int insert(const std::string& path, int flags, int disk_index)
    {
        // Free previous ROM data
        if (_nofrendo_rom) {
            free(_nofrendo_rom);
            _nofrendo_rom = nullptr;
        }
        
        printf("NSF Player inserting NSF from SPIFFS: %s\n", path.c_str());

        // Load NSF from SPIFFS
        int nsf_size;
        _nofrendo_rom = load_nsf_from_spiffs(path.c_str(), &nsf_size);
        if (!_nofrendo_rom) {
            printf("NSF Player can't load NSF from SPIFFS: %s\n", path.c_str());
            return -1;
        }

        printf("NSF Player loaded NSF %s: %d bytes\n", path.c_str(), nsf_size);
        
        // Parse NSF header
        if (!parse_nsf_header(_nofrendo_rom, nsf_size)) {
            printf("NSF header parsing failed\n");
            free(_nofrendo_rom);
            _nofrendo_rom = nullptr;
            return -1;
        }

        // Try to initialize NES emulation context first
        printf("NSF: Starting NSF initialization with basic NES context...\n");
        
        // Create a minimal dummy ROM for NES initialization
        uint8_t dummy_rom[16] = {
            'N', 'E', 'S', 0x1A,  // iNES header
            1, 0, 0, 0,            // 1 PRG bank, no CHR, mapper 0
            0, 0, 0, 0, 0, 0, 0, 0 // padding
        };
        
        // Temporarily replace ROM data for basic NES init
        uint8_t* original_rom = _nofrendo_rom;
        _nofrendo_rom = dummy_rom;
        
        // Initialize basic NES system 
        nes_emulate_init("dummy.nes", width, height);
        
        // Restore original NSF data
        _nofrendo_rom = original_rom;
        
        printf("NSF: Basic NES initialization completed\n");
        
        // Check if NES context is now available
        nes_t* nes_ctx = nes_getcontextptr();
        if (!nes_ctx || !nes_ctx->cpu) {
            printf("NSF: Still no NES context after initialization\n");
            return -1;
        }
        
        printf("NSF: NES context available - CPU: %p, APU: %p\n", nes_ctx->cpu, nes_ctx->apu);
        
        // Setup APU memory handlers
        printf("NSF: Setting up APU memory handlers...\n");
        if (!nsf_setup_apu_handlers()) {
            printf("NSF: Failed to setup APU handlers\n");
            return -1;
        }
        
        // Detect and setup NSF mapper after NES initialization
        int mapper_number = detect_nsf_mapper();
        if (!setup_nsf_mapper(mapper_number)) {
            printf("NSF: Failed to setup mapper %d\n", mapper_number);
            // Continue anyway with basic NES initialization
        }

        // Initialize default song (starting song from NSF header)
        if (!nsf_init_song(_nsf_header.starting_song)) {
            printf("NSF: Failed to initialize starting song %d\n", _nsf_header.starting_song);
            return -1;
        }

        // Execute INIT routine after NES system is fully initialized
        printf("NSF: Executing INIT routine...\n");
        nsf_execute_init_routine();   // Execute INIT routine safely
        return 0;
    }

    virtual int update()
    {
        if (_nofrendo_rom && _nsf_initialized) {
            _lines = NULL;
            
            // Set up CPU for PLAY routine only once per song
            if (!_play_setup_done) {
                nsf_setup_play();
                _play_setup_done = true;
            }
            
            // Execute NSF PLAY routine safely
            nsf_execute_play_routine();
        }
        return 0;
    }

    virtual uint8_t** video_buffer()
    {
        return _lines;
    }
    
    virtual int audio_buffer(int16_t* b, int len)
    {
        int n = frame_sample_count();
        //printf("frame_sample_count:%d\n",n);
        if (nes_sound_cb) {
            //printf("nes_sound_cb:%p\n",nes_sound_cb);
            nes_sound_cb(b,n);  // 8 bit unsigned
            uint8_t* b8 = (uint8_t*)b;
            for (int i = n-1; i >= 0; i--)
                b[i] = (b8[i] ^ 0x80) << 8;  // turn it back into signed 16
        }
        else
            memset(b,0,2*n);
        return n;
    }

    
    virtual void gen_palettes() {};
    virtual const uint32_t* ntsc_palette() { return 0; };
    virtual const uint32_t* pal_palette() { return 0; };
    virtual const uint32_t* rgb_palette() { return 0; };
    virtual int info(const std::string& file, std::vector<std::string>& strs) { return -1; };
    virtual void hid(const uint8_t* d, int len) {};
    virtual void key(int keycode, int pressed, int mods)
    {
        if (!pressed) return; // キー押下時のみ処理
        
        switch (keycode) {
            // NSF song selection (number keys 1-5)
            case 30: nsf_change_song(1); break; // 1 key
            case 31: nsf_change_song(2); break; // 2 key  
            case 32: nsf_change_song(3); break; // 3 key
            case 33: nsf_change_song(4); break; // 4 key
            case 34: nsf_change_song(5); break; // 5 key
                
            default:
                printf("NSF: Key %d pressed (use 1-5 for song selection)\n", keycode);
                break;
        }
    }
};

Emu* NewNsfplayer(int ntsc)
{
    return new EmuNsfPlay(ntsc);
}
