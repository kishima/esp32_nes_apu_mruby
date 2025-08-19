#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "nsf.h"
#include "cpu6502.h"
#include "apu_stub.h"

// Include flag definitions directly since they're not in the header
#define FLAG_R 0x20  // Reserved flag

// Global variables
static NSFPlayer nsf_player;
static CPU6502 cpu;
static volatile int running = 1;

// Signal handler for clean exit
void signal_handler(int sig) {
    running = 0;
}

// Initialize NSF player system
void init_system() {
    // Initialize CPU
    cpu_init(&cpu);
    
    // Initialize APU stub
    apu_init();
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
}

// Run INIT routine for NSF
void run_init(NSFPlayer *player, uint8_t song_num) {
    printf("\nRunning INIT for song %d...\n", song_num);
    
    // Load NSF data into CPU memory
    cpu_load_prg(&cpu, player->data, player->data_size, player->header.load_addr);
    
    // Clear RAM and initialize CPU state manually for NSF
    memset(cpu.ram, 0, sizeof(cpu.ram));
    cpu.sp = 0xFD;
    cpu.p = FLAG_R | FLAG_I;  // Reserved flag always set, interrupts disabled
    
    // Set up for INIT call
    cpu.a = song_num - 1;  // Song number (0-based)
    cpu.x = 0;  // PAL/NTSC flag (0 = NTSC)
    cpu.y = 0;
    
    // Jump to INIT address directly (NSF doesn't use reset vector)
    cpu.pc = player->header.init_addr;
    
    printf("Calling INIT at $%04X with A=%02X, X=%02X\n", cpu.pc, cpu.a, cpu.x);
    
    // Run INIT routine (max 100000 cycles to prevent infinite loop)
    uint32_t start_cycles = cpu.cycles;
    uint32_t max_cycles = 100000;
    
    while (cpu.cycles - start_cycles < max_cycles) {
        uint16_t old_pc = cpu.pc;
        cpu_step(&cpu);
        
        // Check if CPU is jammed
        if (cpu.jammed) {
            printf("INIT stopped (CPU jammed at $%04X)\n", old_pc);
            break;
        }
        
        // Check if we hit an RTS (return from INIT)
        if (cpu_read(old_pc) == 0x60) {  // RTS opcode
            printf("INIT completed (RTS at $%04X)\n", old_pc);
            break;
        }
        
        // Check for infinite loop
        if (cpu.pc == old_pc) {
            printf("INIT completed (infinite loop at $%04X)\n", old_pc);
            break;
        }
    }
    
    printf("INIT finished after %d cycles\n", cpu.cycles - start_cycles);
    
}

// Run PLAY routine for NSF
void run_play(NSFPlayer *player) {
    // Save current CPU state
    uint16_t saved_pc = cpu.pc;
    uint8_t saved_sp = cpu.sp;
    
    // Set up JSR to PLAY address
    // Push return address to stack (dummy address)
    uint16_t return_addr = 0xFFFF;  // Dummy return address
    cpu_write(0x100 + cpu.sp, (return_addr >> 8) & 0xFF);  // High byte
    cpu.sp--;
    cpu_write(0x100 + cpu.sp, return_addr & 0xFF);         // Low byte
    cpu.sp--;
    
    // Jump to PLAY address
    cpu.pc = player->header.play_addr;
    
    // Run PLAY routine for a limited number of cycles
    // NTSC frame = ~29780 cycles, use a smaller portion for PLAY routine
    uint32_t max_cycles = 5000;  // Reasonable limit for PLAY routine per frame
    uint32_t start_cycles = cpu.cycles;
    bool completed_normally = false;
    
    while (cpu.cycles - start_cycles < max_cycles) {
        uint16_t old_pc = cpu.pc;
        uint8_t opcode = cpu_read(cpu.pc);
        
        // Execute the instruction
        cpu_step(&cpu);
        
        // Check if CPU is jammed
        if (cpu.jammed) {
            if (cpu.debug_mode) {
                printf("PLAY routine stopped (CPU jammed at $%04X)\n", old_pc);
            }
            break;
        }
        
        // Check if we executed an RTS and returned to dummy address
        if (opcode == 0x60) {  // RTS
            // Only check for our specific dummy return address (top-level RTS)
            if (cpu.pc == return_addr) {
                if (cpu.debug_mode) {
                    printf("PLAY routine completed normally after %d cycles\n", 
                           cpu.cycles - start_cycles);
                }
                completed_normally = true;
                break;
            }
            // Ignore other RTS instructions (from subroutines within PLAY)
        }
        
        // Check for infinite loop (PC didn't change)
        if (cpu.pc == old_pc) {
            if (cpu.debug_mode) {
                printf("PLAY routine infinite loop detected at $%04X\n", old_pc);
            }
            break;
        }
    }
    
    if (!completed_normally && cpu.debug_mode) {
        printf("PLAY routine timed out after %d cycles\n", cpu.cycles - start_cycles);
    }
    
    // Restore CPU state for next frame
    cpu.pc = saved_pc;
    cpu.sp = saved_sp;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <nsf_file> [song_number]\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    uint8_t song_num = 1;
    
    if (argc >= 3) {
        song_num = atoi(argv[2]);
    }
    
    printf("NSF Player - Loading %s\n", filename);
    
    // Initialize system
    init_system();
    
    // Load NSF file
    if (!nsf_load(&nsf_player, filename)) {
        printf("Failed to load NSF file\n");
        return 1;
    }
    
    // Print NSF information
    nsf_print_info(&nsf_player);
    
    // Validate song number
    if (song_num < 1 || song_num > nsf_player.header.total_songs) {
        printf("Invalid song number. Using song 1\n");
        song_num = 1;
    }
    
    // Initialize the selected song
    if (!nsf_init(&nsf_player, song_num)) {
        printf("Failed to initialize NSF\n");
        nsf_free(&nsf_player);
        return 1;
    }
    
    // Enable debug mode for INIT and first few frames
    cpu.debug_mode = true;
    
    // Run INIT routine
    run_init(&nsf_player, song_num);
    
    // Main playback loop
    printf("\nStarting playback... Press Ctrl+C to stop\n");
    printf("===============================================\n");
    
    int frame_count = 0;
    nsf_player.is_playing = true;

    cpu.debug_mode = true;

    while (running && frame_count < 100000) {  // Run for 100 frames as a test
        printf("\n--- Frame %d ---\n", frame_count);
        
        // Run PLAY routine
        run_play(&nsf_player);
        
        // Check if CPU is jammed
        if (cpu.jammed) {
            printf("\nCPU jammed, stopping playback\n");
            break;
        }
        
        // Simulate frame timing (60 fps = ~16.67ms per frame)
        usleep(16667);
        
        frame_count++;
        
        // Disable debug after first 10 frames to reduce output
        // if (frame_count > 10) {
        //     cpu.debug_mode = false;
        // }
    }
    
    printf("\nPlayback stopped after %d frames\n", frame_count);
    
    // Clean up
    nsf_player.is_playing = false;
    nsf_free(&nsf_player);
    
    return 0;
}