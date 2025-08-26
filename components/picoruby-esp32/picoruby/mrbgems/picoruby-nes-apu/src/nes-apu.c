#include "apu_if.h"

#include <mrubyc.h>
#include <stdio.h>
#include <stdlib.h>

#define NTSC_SAMPLE 262

// APU initialized flag
static int apu_initialized = 0;

// Initialize APU
static void
c__init(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (!apu_initialized) {
        apuif_init();
        apu_initialized = 1;
    }
}

// Write to APU register
static void
c_write_reg(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc != 2) {
        mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
        return;
    }
    
    if (v[1].tt != MRBC_TT_INTEGER || v[2].tt != MRBC_TT_INTEGER) {
        mrbc_raise(vm, MRBC_CLASS(TypeError), "arguments must be integers");
        return;
    }
    
    uint32_t address = mrbc_integer(v[1]);
    uint8_t value = mrbc_integer(v[2]) & 0xFF;
    
    apuif_write_reg(address, value);
}

// Read from APU register
static void
c_read_reg(mrbc_vm *vm, mrbc_value *v, int argc)
{
    if (argc != 1) {
        mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
        return;
    }
    
    if (v[1].tt != MRBC_TT_INTEGER) {
        mrbc_raise(vm, MRBC_CLASS(TypeError), "argument must be integer");
        return;
    }
    
    uint32_t address = mrbc_integer(v[1]);
    uint8_t value = apuif_read_reg(address);
    
    SET_INT_RETURN(value);
}

// Process audio frame
static void
c_process(mrbc_vm *vm, mrbc_value *v, int argc)
{
    static int16_t abuffer[(NTSC_SAMPLE+1)*2];

    int samples = apuif_process(abuffer, sizeof(abuffer));

    apuif_audio_write(abuffer,samples,1);
    SET_INT_RETURN(samples);
}

// // Enable/disable channels
// static void
// c_set_channel_enable(mrbc_vm *vm, mrbc_value *v, int argc)
// {
//     if (argc != 1) {
//         mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
//         return;
//     }
    
//     if (v[1].tt != MRBC_TT_INTEGER) {
//         mrbc_raise(vm, MRBC_CLASS(TypeError), "argument must be integer");
//         return;
//     }
    
//     uint8_t enable_mask = mrbc_integer(v[1]) & 0x1F;
//     apuif_write_reg(0x4015, enable_mask);
// }

void
mrbc_nes_apu_init(mrbc_vm *vm)
{
    mrbc_class *mrbc_class_NesApu = mrbc_define_class(vm, "NesApu", mrbc_class_object);
    
    // Instance methods
    mrbc_define_method(vm, mrbc_class_NesApu, "_init", c__init);
    mrbc_define_method(vm, mrbc_class_NesApu, "write_reg", c_write_reg);
    mrbc_define_method(vm, mrbc_class_NesApu, "read_reg", c_read_reg);
    mrbc_define_method(vm, mrbc_class_NesApu, "process", c_process);
    
}