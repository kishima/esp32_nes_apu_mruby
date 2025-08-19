#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>
#include <stdbool.h>

// CPU Flags
#define FLAG_C 0x01  // Carry
#define FLAG_Z 0x02  // Zero
#define FLAG_I 0x04  // Interrupt Disable
#define FLAG_D 0x08  // Decimal Mode (not used in NES)
#define FLAG_B 0x10  // Break
#define FLAG_R 0x20  // Reserved (always 1)
#define FLAG_V 0x40  // Overflow
#define FLAG_N 0x80  // Negative

// CPU Context
typedef struct {
    uint16_t pc;     // Program Counter
    uint8_t sp;      // Stack Pointer
    uint8_t a;       // Accumulator
    uint8_t x;       // X Register
    uint8_t y;       // Y Register
    uint8_t p;       // Processor Status
    
    // Memory
    uint8_t ram[0x800];     // 2KB internal RAM
    uint8_t *prg_rom;       // PRG ROM pointer
    uint32_t prg_size;      // PRG ROM size
    uint16_t load_addr;     // NSF load address
    
    // Timing
    uint32_t cycles;        // Total CPU cycles
    
    // Debug
    bool debug_mode;        // Enable debug output
} CPU6502;

// Memory access callbacks
typedef uint8_t (*mem_read_func)(uint16_t addr);
typedef void (*mem_write_func)(uint16_t addr, uint8_t value);

// Global memory callbacks
extern mem_read_func cpu_mem_read;
extern mem_write_func cpu_mem_write;

// Function prototypes
void cpu_init(CPU6502 *cpu);
void cpu_reset(CPU6502 *cpu);
void cpu_step(CPU6502 *cpu);
void cpu_run(CPU6502 *cpu, uint32_t cycles);
void cpu_nmi(CPU6502 *cpu);
void cpu_irq(CPU6502 *cpu);

// Memory functions
uint8_t cpu_read(uint16_t addr);
void cpu_write(uint16_t addr, uint8_t value);
void cpu_load_prg(CPU6502 *cpu, uint8_t *data, uint32_t size, uint16_t load_addr);

// Debug functions
void cpu_print_state(CPU6502 *cpu);
void cpu_disassemble(uint16_t addr);

#endif // CPU6502_H