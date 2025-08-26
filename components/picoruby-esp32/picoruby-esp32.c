#include <inttypes.h>

#include "picoruby.h"

#include <mrubyc.h>
#include "mrb/main_task.c"
#ifndef HEAP_SIZE
#define HEAP_SIZE (1024 * 1024 + 1024 * 512)
#endif

#include "esp_heap_caps.h"
void print_psram_stats(void)
{
    // PSRAM(外部RAM)に限定した各種サイズ
    size_t total   = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t minfree = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);      // 起動以降の最小残量(水位)
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);     // 一度に取れる最大連続ブロック

    printf("PSRAM total     : %u bytes\n", (unsigned)total);
    printf("PSRAM free      : %u bytes\n", (unsigned)free);
    printf("PSRAM min free  : %u bytes (low-watermark)\n", (unsigned)minfree);
    printf("PSRAM max alloc : %u bytes (largest contiguous)\n", (unsigned)largest);
}

static uint8_t* heap_pool;

void picoruby_esp32(void)
{
  printf("use PSRAM for mruby heap\n");
  print_psram_stats();
  heap_pool = heap_caps_malloc(HEAP_SIZE, MALLOC_CAP_SPIRAM);
  printf("heap_pool=%p\n",heap_pool);
  print_psram_stats();
  mrbc_init(heap_pool, HEAP_SIZE);

  mrbc_tcb *main_tcb = mrbc_create_task(main_task, 0);
  mrbc_set_task_name(main_tcb, "main_task");
  mrbc_vm *vm = &main_tcb->vm;

  picoruby_init_require(vm);
  mrbc_run();
}
