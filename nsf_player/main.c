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
    // Jump to PLAY address
    cpu.pc = player->header.play_addr;
    
    // Run PLAY routine (one frame worth of cycles)
    // NTSC: ~29780 cycles per frame (1789773 Hz / 60 fps)
    uint32_t cycles_per_frame = 29780;
    uint32_t start_cycles = cpu.cycles;
    
    while (cpu.cycles - start_cycles < cycles_per_frame) {
        uint16_t old_pc = cpu.pc;
        cpu_step(&cpu);
        
        // Check if we hit an RTS or RTI
        uint8_t opcode = cpu_read(old_pc);
        if (opcode == 0x60 || opcode == 0x40) {  // RTS or RTI
            break;
        }
        
        // Check for infinite loop
        if (cpu.pc == old_pc) {
            break;
        }
    }
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
    
    // Run INIT routine
    run_init(&nsf_player, song_num);
    
    // Enable debug mode for first few frames
    cpu.debug_mode = true;
    
    // Main playback loop
    printf("\nStarting playback... Press Ctrl+C to stop\n");
    printf("===============================================\n");
    
    int frame_count = 0;
    nsf_player.is_playing = true;
    
    while (running && frame_count < 10) {  // Run for 10 frames as a test
        printf("\n--- Frame %d ---\n", frame_count);
        
        // Run PLAY routine
        run_play(&nsf_player);
        
        // Simulate frame timing (60 fps = ~16.67ms per frame)
        usleep(16667);
        
        frame_count++;
        
        // Disable debug after first 3 frames
        if (frame_count > 3) {
            cpu.debug_mode = false;
        }
    }
    
    printf("\nPlayback stopped after %d frames\n", frame_count);
    
    // Clean up
    nsf_player.is_playing = false;
    nsf_free(&nsf_player);
    
    return 0;
}