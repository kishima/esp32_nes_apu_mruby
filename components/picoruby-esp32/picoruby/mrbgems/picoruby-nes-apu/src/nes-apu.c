#include "apu_if.h"

#include <mrubyc.h>

static void
c__init(mrbc_vm *vm, mrbc_value *v, int argc)
{

}

void
mrbc_nes_apu_init(mrbc_vm *vm)
{
  mrbc_class *mrbc_class_SPI = mrbc_define_class(vm, "NES_APU", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_SPI, "_init", c__init);
}
