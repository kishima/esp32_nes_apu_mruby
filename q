[1mdiff --git a/.gitignore b/.gitignore[m
[1mindex b4d962f..5d89914 100644[m
[1m--- a/.gitignore[m
[1m+++ b/.gitignore[m
[36m@@ -8,4 +8,6 @@[m [mcomponents/mruby_component/esp32_build_config.rb.lock[m
 dependencies.lock[m
 ref/[m
 spiffs_data/nsf/test.nsf[m
[32m+[m[32mspiffs_data/nsf_local/[m
 log/[m
[32m+[m[32mtemp/[m
\ No newline at end of file[m
[1mdiff --git a/components/apu_emu/src/emu_nofrendo.cpp b/components/apu_emu/src/emu_nofrendo.cpp[m
[1mindex 4ae4cd0..5a983f0 100644[m
[1m--- a/components/apu_emu/src/emu_nofrendo.cpp[m
[1m+++ b/components/apu_emu/src/emu_nofrendo.cpp[m
[36m@@ -32,6 +32,15 @@[m [mextern "C" {[m
 #include "nofrendo/event.h"[m
 #include "nofrendo/nes.h"[m
 #include "nofrendo/nes_apu.h"[m
[32m+[m
[32m+[m[32m// APU register write tracking[m
[32m+[m[32mstatic int apu_write_count = 0;[m
[32m+[m[32mstatic void track_apu_write(uint16_t addr, uint8_t value) {[m
[32m+[m[32m    if (apu_write_count < 10) {  // Log first 10 writes[m
[32m+[m[32m        printf("APU_WRITE[%d]: $%04X = $%02X\n", apu_write_count, addr, value);[m
[32m+[m[32m    }[m
[32m+[m[32m    apu_write_count++;[m
[32m+[m[32m}[m
 };[m
 #include "math.h"[m
 [m
[36m@@ -536,13 +545,47 @@[m [mpublic:[m
             return;[m
         }[m
         [m
[32m+[m[32m        // ZEROページとスタックページ、RAMページを初期化（NSF楽曲が正常動作するため）[m
[32m+[m[32m        static uint8_t zero_page[256];[m
[32m+[m[32m        static uint8_t stack_page[256];[m[41m [m
[32m+[m[32m        static uint8_t ram_page_2[256];   // $0200-$02FF[m
[32m+[m[32m        static uint8_t ram_page_3[256];   // $0300-$03FF[m
[32m+[m[32m        memset(zero_page, 0, sizeof(zero_page));[m
[32m+[m[32m        memset(stack_page, 0, sizeof(stack_page));[m
[32m+[m[32m        memset(ram_page_2, 0, sizeof(ram_page_2));[m
[32m+[m[32m        memset(ram_page_3, 0, sizeof(ram_page_3));[m
[32m+[m[41m        [m
[32m+[m[32m        // KingKong.nsfのPLAYルーチンが正常動作するように、ZEROページを音楽用に設定[m
[32m+[m[32m        zero_page[0x00] = 0xFF;  // 確実にBMI分岐を実行させる[m
[32m+[m[32m        zero_page[0x01] = 0x00;  // 音楽フレームカウンタを0にリセット[m
[32m+[m[32m        zero_page[0x02] = 0x01;  // 音楽有効フラグ[m
[32m+[m[32m        zero_page[0x03] = 0x00;  // テンポカウンタ[m
[32m+[m[32m        zero_page[0x04] = 0x00;  // パターンインデックス[m
[32m+[m[32m        printf("NSF: Set ZEROページ[$00-$04] for music sequence control\n");[m
[32m+[m[41m        [m
[32m+[m[32m        cpu_ctx->mem_page[0x00] = zero_page;   // ZEROページ[m
[32m+[m[32m        cpu_ctx->mem_page[0x01] = stack_page;  // スタックページ[m
[32m+[m[32m        cpu_ctx->mem_page[0x02] = ram_page_2;  // RAMページ[m[41m [m
[32m+[m[32m        cpu_ctx->mem_page[0x03] = ram_page_3;  // RAMページ[m
[32m+[m[41m        [m
[32m+[m[32m        printf("NSF: Initialized memory pages $0000-$03FF\n");[m
[32m+[m
         // NSFデータをメモリページに直接マップ[m
         if (_nofrendo_rom) {[m
             uint8_t* nsf_data = _nofrendo_rom + sizeof(NSFHeader);[m
             size_t nsf_data_size = _nsf_size - sizeof(NSFHeader);[m
             [m
[31m-            // $8000ページにNSFデータをマップ[m
[31m-            cpu_ctx->mem_page[0x8000 >> NES6502_BANKSHIFT] = nsf_data;[m
[32m+[m[32m            // NSFデータを全メモリ範囲にマップ ($8000-$FFFF)[m
[32m+[m[32m            // 32KBのNSFデータを各4KBページに分割してマップ[m
[32m+[m[32m            for (int page = 0x8; page <= 0xF; page++) {[m
[32m+[m[32m                int offset = (page - 0x8) * 0x1000;  // 各ページは4KB[m
[32m+[m[32m                if (offset < nsf_data_size) {[m
[32m+[m[32m                    cpu_ctx->mem_page[page] = nsf_data + offset;[m
[32m+[m[32m                } else {[m
[32m+[m[32m                    // データが不足している場合は最初のページをミラー[m
[32m+[m[32m                    cpu_ctx->mem_page[page] = nsf_data;[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
             [m
             // APUメモリページの設定[m
             static uint8_t apu_memory_page[256];[m
[36m@@ -606,6 +649,18 @@[m [mpublic:[m
         static uint32_t play_count = 0;[m
         play_count++;[m
         [m
[32m+[m[32m        // 最初の数回はPLAYルーチン実行をログ出力[m
[32m+[m[32m        if (play_count <= 5) {[m
[32m+[m[32m            printf("NSF: Executing PLAY routine #%lu at $%04X\n", play_count, _nsf_header.play_addr);[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
[32m+[m[32m        // 60フレーム（1秒）ごとにINITルーチンを再実行して楽曲を再初期化[m
[32m+[m[32m        if (play_count % 60 == 1 && play_count > 1) {[m
[32m+[m[32m            printf("NSF: Re-executing INIT routine for song refresh (frame %lu)\n", play_count);[m
[32m+[m[32m            nsf_execute_init_routine();[m
[32m+[m[32m            return;  // この回はINITのみ実行[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
         // APUコンテキストの確認[m
         nes_t* nes_ctx = nes_getcontextptr();[m
         if (!nes_ctx || !nes_ctx->apu) {[m
[36m@@ -623,11 +678,40 @@[m [mpublic:[m
         // PLAYルーチンのためにPCとスタックのみ設定[m
         play_ctx.pc_reg = _nsf_header.play_addr;  // PLAY address[m
         play_ctx.s_reg = 0xFF;  // Empty stack for NSF termination detection[m
[32m+[m[32m        play_ctx.p_reg = 0x04 | 0x02 | 0x20;  // I_FLAG | Z_FLAG | R_FLAG (クリアな状態)[m
[32m+[m[41m        [m
[32m+[m[32m        // ZEROページを再設定（PLAYルーチンで音楽制御フラグを保証）[m
[32m+[m[32m        static uint8_t zero_page_play[256];[m
[32m+[m[32m        static bool first_setup = true;[m
[32m+[m[32m        if (first_setup) {[m
[32m+[m[32m            memset(zero_page_play, 0, sizeof(zero_page_play));[m
[32m+[m[32m            zero_page_play[0x00] = 0xFF;  // 確実にBMI分岐を実行させる[m
[32m+[m[32m            zero_page_play[0x02] = 0x01;  // 音楽有効フラグ[m
[32m+[m[32m            first_setup = false;[m
[32m+[m[32m        }[m
[32m+[m[32m        // フレームカウンタは毎回インクリメント（音楽進行のため）[m
[32m+[m[32m        zero_page_play[0x01] = (play_count - 1) & 0xFF;  // フレームカウンタ[m
[32m+[m[32m        play_ctx.mem_page[0x00] = zero_page_play;[m
[32m+[m[41m        [m
[32m+[m[32m        if (play_count <= 3) {[m
[32m+[m[32m            printf("NSF PLAY[%u]: Set ZEROページ[$00-$04] frame=%d\n", play_count, zero_page_play[0x01]);[m
[32m+[m[32m        }[m
         [m
         // メモリページを確保（NSFデータ部分）[m
         if (_nofrendo_rom) {[m
             uint8_t* nsf_data = _nofrendo_rom + sizeof(NSFHeader);[m
[31m-            play_ctx.mem_page[8] = nsf_data;  // $8000-$8FFF[m
[32m+[m[32m            size_t nsf_data_size = _nsf_size - sizeof(NSFHeader);[m
[32m+[m[41m            [m
[32m+[m[32m            // NSFデータを全メモリ範囲にマップ ($8000-$FFFF)[m
[32m+[m[32m            for (int page = 0x8; page <= 0xF; page++) {[m
[32m+[m[32m                int offset = (page - 0x8) * 0x1000;  // 各ページは4KB[m
[32m+[m[32m                if (offset < nsf_data_size) {[m
[32m+[m[32m                    play_ctx.mem_page[page] = nsf_data + offset;[m
[32m+[m[32m                } else {[m
[32m+[m[32m                    // データが不足している場合は最初のページをミラー[m
[32m+[m[32m                    play_ctx.mem_page[page] = nsf_data;[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
         }[m
         [m
         // デバッグ: 実行前の状態確認[m
[36m@@ -639,6 +723,18 @@[m [mpublic:[m
         // 状態を設定（RAMやAPU状態は保持される）[m
         nes6502_setcontext(&play_ctx);[m
         [m
[32m+[m[32m        // setcontext後にZEROページを直接書き込み（CPUコンテキストの上書き対策）[m
[32m+[m[32m        nes6502_context* current_cpu = nes_getcontextptr()->cpu;[m
[32m+[m[32m        if (current_cpu && current_cpu->mem_page[0x00]) {[m
[32m+[m[32m            current_cpu->mem_page[0x00][0x00] = 0xFF;  // 確実にBMI分岐するため0xFFに設定[m
[32m+[m[32m            current_cpu->mem_page[0x00][0x01] = (play_count - 1) & 0xFF;  // フレームカウンタ[m
[32m+[m[32m            current_cpu->mem_page[0x00][0x02] = 0x01;  // 音楽有効フラグ[m
[32m+[m[32m            if (play_count <= 3) {[m
[32m+[m[32m                printf("NSF PLAY[%u]: Direct write ZEROページ frame=%d after setcontext\n",[m[41m [m
[32m+[m[32m                       play_count, current_cpu->mem_page[0x00][0x01]);[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
         // setcontext後の確認（正しい方法で確認）[m
         if (debug_print && play_count <= 20) {[m
             static nes6502_context verify_ctx;[m
[36m@@ -647,12 +743,44 @@[m [mpublic:[m
                    play_count, verify_ctx.pc_reg, play_ctx.pc_reg);[m
         }[m
         [m
[31m-        // NSF PLAY実行[m
[31m-        int cycles_executed = nes6502_execute(500);[m
[32m+[m[32m        // NSF PLAY実行（KingKong.nsfは非常に複雑な音楽処理のため大量のサイクル数）[m
[32m+[m[32m        int cycles_executed = nes6502_execute(10000);[m
         [m
[31m-        // デバッグ: 実行サイクル数を確認[m
[31m-        if (debug_print && play_count <= 20) {[m
[31m-            printf("NSF PLAY[%u]: Executed %d cycles\n", play_count, cycles_executed);[m
[32m+[m[32m        // デバッグ: 実行サイクル数とPCの変化を確認[m[41m  [m
[32m+[m[32m        if (play_count <= 10) {[m
[32m+[m[32m            nes6502_context current_state;[m
[32m+[m[32m            nes6502_getcontext(&current_state);[m
[32m+[m[32m            printf("NSF PLAY[%u]: Executed %d cycles, final PC=$%04X\n",[m[41m [m
[32m+[m[32m                   play_count, cycles_executed, current_state.pc_reg);[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
[32m+[m[32m        // 早期終了する場合の詳細デバッグ[m
[32m+[m[32m        if ((cycles_executed == 39 || cycles_executed == 22) && play_count <= 5) {[m
[32m+[m[32m            uint8_t* play_code = nes_getcontextptr()->cpu->mem_page[0xB];  // $BE00 page[m
[32m+[m[32m            uint8_t* zero_page_check = nes_getcontextptr()->cpu->mem_page[0x00];  // ZEROページ[m
[32m+[m[32m            if (play_code) {[m
[32m+[m[32m                printf("NSF PLAY CODE at $BE00: %02X %02X %02X %02X %02X %02X %02X %02X\n",[m
[32m+[m[32m                       play_code[0xE00], play_code[0xE01], play_code[0xE02], play_code[0xE03],[m
[32m+[m[32m                       play_code[0xE04], play_code[0xE05], play_code[0xE06], play_code[0xE07]);[m
[32m+[m[41m                       [m
[32m+[m[32m                // $BE08以降のコードも確認[m
[32m+[m[32m                printf("NSF PLAY CODE at $BE08: %02X %02X %02X %02X %02X %02X %02X %02X\n",[m
[32m+[m[32m                       play_code[0xE08], play_code[0xE09], play_code[0xE0A], play_code[0xE0B],[m
[32m+[m[32m                       play_code[0xE0C], play_code[0xE0D], play_code[0xE0E], play_code[0xE0F]);[m
[32m+[m[32m            }[m
[32m+[m[32m            if (zero_page_check) {[m
[32m+[m[32m                printf("NSF ZEROページ[$00] = 0x%02X (should be 0xFF)\n", zero_page_check[0x00]);[m
[32m+[m[32m            }[m
[32m+[m[41m            [m
[32m+[m[32m            // CPUフラグの状態も確認[m
[32m+[m[32m            nes6502_context final_ctx;[m
[32m+[m[32m            nes6502_getcontext(&final_ctx);[m
[32m+[m[32m            printf("NSF PLAY[%u]: Final CPU flags: N=%d Z=%d C=%d P=$%02X\n",[m[41m [m
[32m+[m[32m                   play_count,[m[41m [m
[32m+[m[32m                   (final_ctx.p_reg & 0x80) ? 1 : 0,  // N flag[m
[32m+[m[32m                   (final_ctx.p_reg & 0x02) ? 1 : 0,  // Z flag[m
[32m+[m[32m                   (final_ctx.p_reg & 0x01) ? 1 : 0,  // C flag[m
[32m+[m[32m                   final_ctx.p_reg);[m
         }[m
     }[m
 [m
[36m@@ -685,7 +813,7 @@[m [mpublic:[m
         [m
         // Set NSF-specific CPU registers[m
         cpu_ctx->pc_reg = _nsf_header.init_addr;  // INIT routine address[m
[31m-        cpu_ctx->a_reg = song_number - 1;         // Song number (0-based)[m
[32m+[m[32m        cpu_ctx->a_reg = song_number - 1;         // Song number (0-based)[m[41m [m
         cpu_ctx->x_reg = 0;                       // 0 = NTSC, 1 = PAL[m
         cpu_ctx->s_reg = 0xFF;                    // Initialize stack pointer[m
         cpu_ctx->p_reg = 0x04 | 0x02 | 0x20;     // I_FLAG | Z_FLAG | R_FLAG[m
[1mdiff --git a/components/apu_emu/src/nofrendo/nes6502.c b/components/apu_emu/src/nofrendo/nes6502.c[m
[1mindex 8542768..6c024e8 100644[m
[1m--- a/components/apu_emu/src/nofrendo/nes6502.c[m
[1m+++ b/components/apu_emu/src/nofrendo/nes6502.c[m
[36m@@ -31,10 +31,10 @@[m
 //#define  NES6502_DISASM[m
 [m
 // デバッグログ制御フラグ[m
[31m-#define CPU_DEBUG       0    // CPU実行詳細ログ[m
[31m-#define MEMORY_DEBUG    0    // メモリアクセスログ[m
[31m-#define JSR_DEBUG       0    // JSR命令詳細ログ[m
[31m-#define OPCODE_DEBUG    0    // オペコード実行ログ[m
[32m+[m[32m#define CPU_DEBUG       1    // CPU実行詳細ログ[m
[32m+[m[32m#define MEMORY_DEBUG    1    // メモリアクセスログ[m
[32m+[m[32m#define JSR_DEBUG       1    // JSR命令詳細ログ[m
[32m+[m[32m#define OPCODE_DEBUG    1    // オペコード実行ログ[m
 [m
 #define  ADD_CYCLES(x) \[m
 { \[m
[36m@@ -1275,6 +1275,15 @@[m [mstatic void mem_writebyte(uint32 address, uint8 value)[m
 {[m
    nes6502_memwrite *mw;[m
 [m
[32m+[m[32m   /* APUアドレス監視 */[m
[32m+[m[32m   if (MEMORY_DEBUG && address >= 0x4000 && address <= 0x4015) {[m
[32m+[m[32m       static int apu_write_count = 0;[m
[32m+[m[32m       if (apu_write_count < 50) {[m
[32m+[m[32m           printf("[MEM_WRITE_APU] addr=$%04X, val=$%02X (count=%d)\n", address, value, apu_write_count);[m
[32m+[m[32m           apu_write_count++;[m
[32m+[m[32m       }[m
[32m+[m[32m   }[m
[32m+[m
    /* RAM */[m
    if (address < 0x800)[m
    {[m
[36m@@ -1385,7 +1394,7 @@[m [muint32 nes6502_getcycles(bool reset_flag)[m
 #define  MIN(a,b)    (((a) < (b)) ? (a) : (b))[m
 #define  OPCODE_BEGIN(xx)  op##xx:[m
 [m
[31m-#define MAX_DEBUG_OPCODE 30[m
[32m+[m[32m#define MAX_DEBUG_OPCODE 200[m
 [m
 #define  OPCODE_END \[m
    if (remaining_cycles <= 0) \[m
[36m@@ -1393,7 +1402,7 @@[m [muint32 nes6502_getcycles(bool reset_flag)[m
    { \[m
       static int opcode_fetch_count = 0; \[m
       uint8 next_opcode = bank_readbyte(PC); \[m
[31m-      if (OPCODE_DEBUG && opcode_fetch_count < MAX_DEBUG_OPCODE ) { \[m
[32m+[m[32m      if (OPCODE_DEBUG && opcode_fetch_count < MAX_DEBUG_OPCODE && PC >= 0xBE00 && PC < 0xBF00) { \[m
          printf("[OPCODE_FETCH] PC=$%04X, opcode=$%02X, remaining_cycles=%d\n", PC, next_opcode, remaining_cycles); \[m
          opcode_fetch_count++; \[m
       } \[m
[36m@@ -1508,7 +1517,7 @@[m [mint nes6502_execute(int timeslice_cycles)[m
    {[m
       static int first_fetch_count = 0;[m
       uint8 first_opcode = bank_readbyte(PC);[m
[31m-      if (OPCODE_DEBUG && first_fetch_count < 50 && PC >= 0x8000 && PC < 0x8100) {[m
[32m+[m[32m      if (OPCODE_DEBUG && first_fetch_count < 50 && PC >= 0xBE00 && PC < 0xBF00) {[m
          printf("[FIRST_FETCH] PC=$%04X, opcode=$%02X, remaining_cycles=%d\n", PC, first_opcode, remaining_cycles);[m
          first_fetch_count++;[m
       }[m
[36m@@ -1738,6 +1747,14 @@[m [mint nes6502_execute(int timeslice_cycles)[m
          OPCODE_END[m
 [m
       OPCODE_BEGIN(30)  /* BMI $nnnn */[m
[32m+[m[32m         if (OPCODE_DEBUG) {[m
[32m+[m[32m             static int bmi_count = 0;[m
[32m+[m[32m             if (bmi_count < 10) {[m
[32m+[m[32m                 printf("[BMI_DEBUG] PC=$%04X, n_flag=0x%02X, N_FLAG=0x%02X, will branch=%s\n",[m[41m [m
[32m+[m[32m                        PC-1, n_flag, N_FLAG, (n_flag & N_FLAG) ? "YES" : "NO");[m
[32m+[m[32m                 bmi_count++;[m
[32m+[m[32m             }[m
[32m+[m[32m         }[m
          BMI();[m
          OPCODE_END[m
 [m
[36m@@ -1890,6 +1907,13 @@[m [mint nes6502_execute(int timeslice_cycles)[m
          OPCODE_END[m
 [m
       OPCODE_BEGIN(60)  /* RTS */[m
[32m+[m[32m         if (OPCODE_DEBUG) {[m
[32m+[m[32m             static int rts_count = 0;[m
[32m+[m[32m             if (rts_count < 10) {[m
[32m+[m[32m                 printf("[RTS_DEBUG] PC=$%04X, S=$%02X, returning to stack\n", PC-1, S);[m
[32m+[m[32m                 rts_count++;[m
[32m+[m[32m             }[m
[32m+[m[32m         }[m
          RTS();[m
          OPCODE_END[m
 [m
[36m@@ -2042,6 +2066,19 @@[m [mint nes6502_execute(int timeslice_cycles)[m
          OPCODE_END[m
 [m
       OPCODE_BEGIN(8D)  /* STA $nnnn */[m
[32m+[m[32m         if (OPCODE_DEBUG) {[m
[32m+[m[32m             uint16 target_addr;[m
[32m+[m[32m             ABSOLUTE_ADDR(target_addr);[m
[32m+[m[32m             if (target_addr >= 0x4000 && target_addr <= 0x4015) {[m
[32m+[m[32m                 static int sta_apu_count = 0;[m
[32m+[m[32m                 if (sta_apu_count < 20) {[m
[32m+[m[32m                     printf("[STA_APU_DEBUG] PC=$%04X, STA $%04X, A=$%02X (count=%d)\n",[m[41m [m
[32m+[m[32m                            PC-3, target_addr, A, sta_apu_count);[m
[32m+[m[32m                     sta_apu_count++;[m
[32m+[m[32m                 }[m
[32m+[m[32m             }[m
[32m+[m[32m             PC -= 2;  // Reset PC for normal STA execution[m
[32m+[m[32m         }[m
          STA(4, ABSOLUTE_ADDR, mem_writebyte, addr);[m
          OPCODE_END[m
 [m
[1mdiff --git a/components/apu_emu/src/nofrendo/nes_apu.c b/components/apu_emu/src/nofrendo/nes_apu.c[m
[1mindex e339875..c1528c0 100644[m
[1m--- a/components/apu_emu/src/nofrendo/nes_apu.c[m
[1m+++ b/components/apu_emu/src/nofrendo/nes_apu.c[m
[36m@@ -36,7 +36,7 @@[m
 #define APU_DEBUG       0    // APU処理詳細ログ[m
 #define CHANNEL_DEBUG   0    // チャンネル状態ログ[m
 #define SAMPLE_DEBUG    0    // サンプル生成ログ[m
[31m-#define APU_WRITE_DEBUG 0    // APU書き込みログ[m
[32m+[m[32m#define APU_WRITE_DEBUG 1    // APU書き込みログ[m
 [m
 #define  APU_OVERSAMPLE[m
 #define  APU_VOLUME_DECAY(x)  ((x) -= ((x) >> 7))[m
[36m@@ -649,9 +649,14 @@[m [mvoid apu_write(uint32 address, uint8 value)[m
    static uint32_t last_write_time = 0;[m
    uint32_t current_time = esp_timer_get_time() / 1000;  // ミリ秒[m
    [m
[31m-   if (APU_WRITE_DEBUG && write_count < 50) {  // 最初の50回のみ表示[m
[31m-       printf("APU_WRITE[%d]: addr=$%04X, val=$%02X (time=%lu ms, delta=%lu ms)\n", [m
[31m-              write_count, address, value, current_time, current_time - last_write_time);[m
[32m+[m[32m   // PLAYルーチン実行中の書き込みを特に監視[m
[32m+[m[32m   static bool play_routine_active = false;[m
[32m+[m[32m   if (write_count > 50) play_routine_active = true;[m
[32m+[m[41m   [m
[32m+[m[32m   if (APU_WRITE_DEBUG && (write_count < 50 || (play_routine_active && write_count < 100))) {[m
[32m+[m[32m       printf("APU_WRITE[%d]: addr=$%04X, val=$%02X (time=%lu ms, delta=%lu ms)%s\n",[m[41m [m
[32m+[m[32m              write_count, address, value, current_time, current_time - last_write_time,[m
[32m+[m[32m              play_routine_active ? " [PLAY]" : " [INIT]");[m
    }[m
    [m
    // 特別なレジスタの場合は詳細情報を表示（チャンネル有効化は常に表示）[m
[1mdiff --git a/components/apu_emu/src/video_out.h b/components/apu_emu/src/video_out.h[m
[1mindex bd80769..c63b541 100644[m
[1m--- a/components/apu_emu/src/video_out.h[m
[1m+++ b/components/apu_emu/src/video_out.h[m
[36m@@ -323,6 +323,7 @@[m [mvoid video_init_hw(int line_width, int samples_per_cc)[m
     //                   v gnd[m
 [m
     // ESP-IDF LEDC configuration for PWM audio (ESP-IDF v5.4 compatible)[m
[32m+[m[32m#if 0[m
     ledc_timer_config_t ledc_timer = {[m
         .speed_mode       = LEDC_HIGH_SPEED_MODE,[m
         .duty_resolution  = LEDC_TIMER_7_BIT,[m
[36m@@ -347,6 +348,31 @@[m [mvoid video_init_hw(int line_width, int samples_per_cc)[m
     ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);[m
     ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);[m
 [m
[32m+[m[32m#else[m
[32m+[m
[32m+[m[32m    ledc_timer_config_t tcfg{};[m
[32m+[m[32m    tcfg.speed_mode      = LEDC_HIGH_SPEED_MODE;[m
[32m+[m[32m    tcfg.duty_resolution = LEDC_TIMER_8_BIT;   // 8bit[m
[32m+[m[32m    tcfg.timer_num       = LEDC_TIMER_0;[m
[32m+[m[32m    tcfg.freq_hz         = 312500;             // 80MHz / 2^8[m
[32m+[m[32m    tcfg.clk_cfg         = LEDC_USE_APB_CLK;[m
[32m+[m[32m    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));[m
[32m+[m
[32m+[m[32m    ledc_channel_config_t ccfg{};[m
[32m+[m[32m    ccfg.gpio_num   = AUDIO_PIN;[m
[32m+[m[32m    ccfg.speed_mode = LEDC_HIGH_SPEED_MODE;[m
[32m+[m[32m    ccfg.channel    = LEDC_CHANNEL_0;[m
[32m+[m[32m    ccfg.intr_type  = LEDC_INTR_DISABLE;[m
[32m+[m[32m    ccfg.timer_sel  = LEDC_TIMER_0;[m
[32m+[m[32m    ccfg.duty       = 0;[m
[32m+[m[32m    ccfg.hpoint     = 0;[m
[32m+[m[32m    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));[m
[32m+[m
[32m+[m[32m    LEDC.channel_group[0].channel[0].conf0.sig_out_en = 1;[m
[32m+[m[32m    LEDC.channel_group[0].channel[0].conf0.clk_en     = 1;[m
[32m+[m
[32m+[m[32m#endif[m
[32m+[m
     // Previous Arduino-style API (Arduino framework):[m
     // ledcSetup(0, 2000000, 7);  // channel, frequency, resolution // 625000 khz is as fast as we go w 7 bitss[m
     // ledcAttachPin(AUDIO_PIN, 0);  // pin, channel[m
[36m@@ -378,6 +404,7 @@[m [minline void IRAM_ATTR audio_sample(uint8_t s)[m
 }[m
 #else[m
 // PWM audio sample (従来実装)[m
[32m+[m[32m#if 0[m
 inline void IRAM_ATTR audio_sample(uint8_t s)[m
 {[m
     auto& reg = LEDC.channel_group[0].channel[0];[m
[36m@@ -386,6 +413,16 @@[m [minline void IRAM_ATTR audio_sample(uint8_t s)[m
     reg.conf1.duty_start = 1; // When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware[m
     reg.conf0.clk_en = 1;[m
 }[m
[32m+[m[32m#else[m
[32m+[m
[32m+[m[32mstatic inline void IRAM_ATTR audio_sample(uint8_t s)[m
[32m+[m[32m{[m
[32m+[m[32m    auto &ch = LEDC.channel_group[0].channel[0];[m
[32m+[m[32m    ch.duty.duty = ((uint32_t)s) << 4;   // 0..255 -> Q8.4[m
[32m+[m[32m    ch.conf1.duty_start = 1;             // 更新トリガ（conf1）[m
[32m+[m[32m}[m
[32m+[m[32m#endif[m
[32m+[m
 #endif[m
 [m
 //  Appendix[m
[1mdiff --git a/main/main.cpp b/main/main.cpp[m
[1mindex bfe7eb9..4a52430 100644[m
[1m--- a/main/main.cpp[m
[1m+++ b/main/main.cpp[m
[36m@@ -129,9 +129,10 @@[m [mvoid emu_task(void* arg)[m
     srand(esp_timer_get_time());[m
 [m
     //emu init[m
[31m-    std::string rom_file = "/nsf/continuous_tone_single.nsf";[m
[32m+[m[32m    //std::string rom_file = "/nsf/continuous_tone_single.nsf";[m
     //std::string rom_file = "/nsf/test.nsf";[m
     //std::string rom_file = "/nsf/minimal_test.nsf";[m
[32m+[m[32m    std::string rom_file = "/nsf_local/KingKong.nsf";[m
     if (_emu->insert(rom_file.c_str(),0,0) != 0) {[m
         printf("Failed to load ROM, suspending emu_task\n");[m
         vTaskSuspend(NULL);  // Suspend this task to prevent crashes[m
[1mdiff --git a/spiffs_data/nsf/continuous_tone.nsf b/spiffs_data/nsf/continuous_tone.nsf[m
[1mdeleted file mode 100644[m
[1mindex ad32b07..0000000[m
Binary files a/spiffs_data/nsf/continuous_tone.nsf and /dev/null differ
