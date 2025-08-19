#include <stdio.h>
#include <string.h>
#include "cpu6502.h"
#include "apu_stub.h"

// Global CPU instance
static CPU6502 *g_cpu = NULL;

// Memory callbacks
mem_read_func cpu_mem_read = NULL;
mem_write_func cpu_mem_write = NULL;

// Opcode names for debugging
static const char* opcode_names[256] = {
    "BRK", "ORA", "???", "SLO", "NOP", "ORA", "ASL", "SLO",  // 00-07
    "PHP", "ORA", "ASL", "???", "NOP", "ORA", "ASL", "SLO",  // 08-0F
    "BPL", "ORA", "???", "SLO", "NOP", "ORA", "ASL", "SLO",  // 10-17
    "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",  // 18-1F
    "JSR", "AND", "???", "RLA", "BIT", "AND", "ROL", "RLA",  // 20-27
    "PLP", "AND", "ROL", "???", "BIT", "AND", "ROL", "RLA",  // 28-2F
    "BMI", "AND", "???", "RLA", "NOP", "AND", "ROL", "RLA",  // 30-37
    "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",  // 38-3F
    "RTI", "EOR", "???", "SRE", "NOP", "EOR", "LSR", "SRE",  // 40-47
    "PHA", "EOR", "LSR", "???", "JMP", "EOR", "LSR", "SRE",  // 48-4F
    "BVC", "EOR", "???", "SRE", "NOP", "EOR", "LSR", "SRE",  // 50-57
    "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",  // 58-5F
    "RTS", "ADC", "???", "RRA", "NOP", "ADC", "ROR", "RRA",  // 60-67
    "PLA", "ADC", "ROR", "???", "JMP", "ADC", "ROR", "RRA",  // 68-6F
    "BVS", "ADC", "???", "RRA", "NOP", "ADC", "ROR", "RRA",  // 70-77
    "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",  // 78-7F
    "NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX",  // 80-87
    "DEY", "NOP", "TXA", "???", "STY", "STA", "STX", "SAX",  // 88-8F
    "BCC", "STA", "???", "???", "STY", "STA", "STX", "SAX",  // 90-97
    "TYA", "STA", "TXS", "???", "SHY", "STA", "SHX", "???",  // 98-9F
    "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX",  // A0-A7
    "TAY", "LDA", "TAX", "???", "LDY", "LDA", "LDX", "LAX",  // A8-AF
    "BCS", "LDA", "???", "LAX", "LDY", "LDA", "LDX", "LAX",  // B0-B7
    "CLV", "LDA", "TSX", "???", "LDY", "LDA", "LDX", "LAX",  // B8-BF
    "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP",  // C0-C7
    "INY", "CMP", "DEX", "???", "CPY", "CMP", "DEC", "DCP",  // C8-CF
    "BNE", "CMP", "???", "DCP", "NOP", "CMP", "DEC", "DCP",  // D0-D7
    "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",  // D8-DF
    "CPX", "SBC", "NOP", "ISB", "CPX", "SBC", "INC", "ISB",  // E0-E7
    "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISB",  // E8-EF
    "BEQ", "SBC", "???", "ISB", "NOP", "SBC", "INC", "ISB",  // F0-F7
    "SED", "SBC", "NOP", "ISB", "NOP", "SBC", "INC", "ISB"   // F8-FF
};

// Memory read function
uint8_t cpu_read(uint16_t addr) {
    if (!g_cpu) return 0;
    
    // Internal RAM (mirrored)
    if (addr < 0x2000) {
        return g_cpu->ram[addr & 0x7FF];
    }
    // PPU registers (not implemented for NSF)
    else if (addr >= 0x2000 && addr < 0x4000) {
        return 0;
    }
    // APU and I/O
    else if (addr >= 0x4000 && addr < 0x4020) {
        if (addr == 0x4015) {
            return apu_read(addr);
        }
        return 0;
    }
    // Cartridge space
    else if (addr >= g_cpu->load_addr) {
        uint32_t offset = addr - g_cpu->load_addr;
        if (g_cpu->prg_rom && offset < g_cpu->prg_size) {
            return g_cpu->prg_rom[offset];
        }
    }
    
    return 0;
}

// Memory write function
void cpu_write(uint16_t addr, uint8_t value) {
    if (!g_cpu) return;
    
    // Internal RAM (mirrored)
    if (addr < 0x2000) {
        g_cpu->ram[addr & 0x7FF] = value;
    }
    // PPU registers (not implemented for NSF)
    else if (addr >= 0x2000 && addr < 0x4000) {
        // Ignore PPU writes
    }
    // APU and I/O
    else if (addr >= 0x4000 && addr < 0x4020) {
        apu_write(addr, value);
    }
    // Cartridge space (usually ROM, but some mappers have RAM here)
    else if (addr >= 0x6000 && addr < 0x8000) {
        // SRAM area - for now, ignore writes
    }
    // NSF ROM area - read-only for safety
    else if (addr >= g_cpu->load_addr) {
        // NSF ROM area is read-only
    }
}

// Load PRG ROM
void cpu_load_prg(CPU6502 *cpu, uint8_t *data, uint32_t size, uint16_t load_addr) {
    cpu->prg_rom = data;
    cpu->prg_size = size;
    cpu->load_addr = load_addr;
    printf("CPU: Loaded %d bytes of PRG ROM at $%04X\n", size, load_addr);
}

// Initialize CPU
void cpu_init(CPU6502 *cpu) {
    memset(cpu, 0, sizeof(CPU6502));
    cpu->sp = 0xFD;  // Stack pointer initial value
    cpu->p = FLAG_R | FLAG_I;  // Reserved flag always set, interrupts disabled
    g_cpu = cpu;
    
    printf("CPU: Initialized\n");
}

// Reset CPU
void cpu_reset(CPU6502 *cpu) {
    cpu->sp = 0xFD;
    cpu->p = FLAG_R | FLAG_I;
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    
    // Read reset vector
    uint16_t reset_vector = cpu_read(0xFFFC) | (cpu_read(0xFFFD) << 8);
    cpu->pc = reset_vector;
    
    printf("CPU: Reset, PC = $%04X\n", cpu->pc);
}

// Stack operations
static void push8(CPU6502 *cpu, uint8_t value) {
    cpu_write(0x100 + cpu->sp, value);
    cpu->sp--;
}

static uint8_t pull8(CPU6502 *cpu) {
    cpu->sp++;
    return cpu_read(0x100 + cpu->sp);
}

static void push16(CPU6502 *cpu, uint16_t value) {
    push8(cpu, (value >> 8) & 0xFF);
    push8(cpu, value & 0xFF);
}

static uint16_t pull16(CPU6502 *cpu) {
    uint16_t lo = pull8(cpu);
    uint16_t hi = pull8(cpu);
    return (hi << 8) | lo;
}

// Flag operations
static void set_flag(CPU6502 *cpu, uint8_t flag, bool value) {
    if (value) {
        cpu->p |= flag;
    } else {
        cpu->p &= ~flag;
    }
}

static bool get_flag(CPU6502 *cpu, uint8_t flag) {
    return (cpu->p & flag) != 0;
}

static void update_nz(CPU6502 *cpu, uint8_t value) {
    set_flag(cpu, FLAG_Z, value == 0);
    set_flag(cpu, FLAG_N, (value & 0x80) != 0);
}

// Print CPU state for debugging
void cpu_print_state(CPU6502 *cpu) {
    printf("PC:%04X A:%02X X:%02X Y:%02X SP:%02X P:%02X [%c%c%c%c%c%c%c] CYC:%d\n",
           cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, cpu->p,
           get_flag(cpu, FLAG_N) ? 'N' : '-',
           get_flag(cpu, FLAG_V) ? 'V' : '-',
           get_flag(cpu, FLAG_B) ? 'B' : '-',
           get_flag(cpu, FLAG_D) ? 'D' : '-',
           get_flag(cpu, FLAG_I) ? 'I' : '-',
           get_flag(cpu, FLAG_Z) ? 'Z' : '-',
           get_flag(cpu, FLAG_C) ? 'C' : '-',
           cpu->cycles);
}

// Execute one instruction
void cpu_step(CPU6502 *cpu) {
    uint8_t opcode = cpu_read(cpu->pc);
    uint16_t addr;
    uint8_t value;
    int8_t offset;
    
    if (cpu->debug_mode) {
        printf("$%04X: %02X (%s) ", cpu->pc, opcode, opcode_names[opcode]);
        cpu_print_state(cpu);
    }
    
    cpu->pc++;
    
    // Simplified opcode implementation (only essential opcodes for NSF)
    switch(opcode) {
        // LDA immediate
        case 0xA9:
            cpu->a = cpu_read(cpu->pc++);
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
            
        // LDX immediate
        case 0xA2:
            cpu->x = cpu_read(cpu->pc++);
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
            
        // LDY immediate
        case 0xA0:
            cpu->y = cpu_read(cpu->pc++);
            update_nz(cpu, cpu->y);
            cpu->cycles += 2;
            break;
            
        // STA absolute
        case 0x8D:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            cpu_write(addr, cpu->a);
            cpu->cycles += 4;
            break;
            
        // STX absolute
        case 0x8E:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            cpu_write(addr, cpu->x);
            cpu->cycles += 4;
            break;
            
        // STY absolute
        case 0x8C:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            cpu_write(addr, cpu->y);
            cpu->cycles += 4;
            break;
            
        // JMP absolute
        case 0x4C:
            cpu->pc = cpu_read(cpu->pc) | (cpu_read(cpu->pc + 1) << 8);
            cpu->cycles += 3;
            break;
            
        // JSR absolute
        case 0x20:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            push16(cpu, cpu->pc - 1);
            cpu->pc = addr;
            cpu->cycles += 6;
            break;
            
        // RTS
        case 0x60:
            cpu->pc = pull16(cpu) + 1;
            cpu->cycles += 6;
            break;
            
        // RTI
        case 0x40:
            cpu->p = pull8(cpu);
            cpu->pc = pull16(cpu);
            cpu->cycles += 6;
            break;
            
        // NOP
        case 0xEA:
            cpu->cycles += 2;
            break;
            
        // SEI
        case 0x78:
            set_flag(cpu, FLAG_I, true);
            cpu->cycles += 2;
            break;
            
        // CLD
        case 0xD8:
            set_flag(cpu, FLAG_D, false);
            cpu->cycles += 2;
            break;
            
        // TXS
        case 0x9A:
            cpu->sp = cpu->x;
            cpu->cycles += 2;
            break;
            
        // BNE
        case 0xD0:
            offset = (int8_t)cpu_read(cpu->pc++);
            if (!get_flag(cpu, FLAG_Z)) {
                cpu->pc += offset;
                cpu->cycles += 3;
            } else {
                cpu->cycles += 2;
            }
            break;
            
        // BEQ
        case 0xF0:
            offset = (int8_t)cpu_read(cpu->pc++);
            if (get_flag(cpu, FLAG_Z)) {
                cpu->pc += offset;
                cpu->cycles += 3;
            } else {
                cpu->cycles += 2;
            }
            break;
            
        // BPL
        case 0x10:
            offset = (int8_t)cpu_read(cpu->pc++);
            if (!get_flag(cpu, FLAG_N)) {
                cpu->pc += offset;
                cpu->cycles += 3;
            } else {
                cpu->cycles += 2;
            }
            break;
            
        // BMI
        case 0x30:
            offset = (int8_t)cpu_read(cpu->pc++);
            if (get_flag(cpu, FLAG_N)) {
                cpu->pc += offset;
                cpu->cycles += 3;
            } else {
                cpu->cycles += 2;
            }
            break;
            
        // INX
        case 0xE8:
            cpu->x++;
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
            
        // DEX
        case 0xCA:
            cpu->x--;
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
            
        // INY
        case 0xC8:
            cpu->y++;
            update_nz(cpu, cpu->y);
            cpu->cycles += 2;
            break;
            
        // DEY
        case 0x88:
            cpu->y--;
            update_nz(cpu, cpu->y);
            cpu->cycles += 2;
            break;
            
        // BRK
        case 0x00:
            push16(cpu, cpu->pc);
            push8(cpu, cpu->p | FLAG_B);
            set_flag(cpu, FLAG_I, true);
            cpu->pc = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8);
            cpu->cycles += 7;
            break;
            
        // CMP immediate
        case 0xC9:
            value = cpu_read(cpu->pc++);
            set_flag(cpu, FLAG_C, cpu->a >= value);
            update_nz(cpu, cpu->a - value);
            cpu->cycles += 2;
            break;
            
        // LDA absolute
        case 0xAD:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            cpu->a = cpu_read(addr);
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
            
        // STA zero page
        case 0x85:
            cpu_write(cpu_read(cpu->pc++), cpu->a);
            cpu->cycles += 3;
            break;
            
        // LDA zero page
        case 0xA5:
            cpu->a = cpu_read(cpu_read(cpu->pc++));
            update_nz(cpu, cpu->a);
            cpu->cycles += 3;
            break;
            
        // PHA
        case 0x48:
            push8(cpu, cpu->a);
            cpu->cycles += 3;
            break;
            
        // PLA
        case 0x68:
            cpu->a = pull8(cpu);
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
            
        // TAX
        case 0xAA:
            cpu->x = cpu->a;
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
            
        // LDA absolute,X
        case 0xBD:
            addr = cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8);
            cpu->a = cpu_read(addr + cpu->x);
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
            
        // CLI
        case 0x58:
            set_flag(cpu, FLAG_I, false);
            cpu->cycles += 2;
            break;
            
        // TXA
        case 0x8A:
            cpu->a = cpu->x;
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
            
        // TYA
        case 0x98:
            cpu->a = cpu->y;
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
            
        default:
            printf("Warning: Unimplemented opcode $%02X at $%04X\n", opcode, cpu->pc - 1);
            cpu->cycles += 2;
            break;
    }
}

// Run CPU for specified number of cycles
void cpu_run(CPU6502 *cpu, uint32_t target_cycles) {
    uint32_t start_cycles = cpu->cycles;
    while (cpu->cycles - start_cycles < target_cycles) {
        cpu_step(cpu);
    }
}

// NMI interrupt
void cpu_nmi(CPU6502 *cpu) {
    push16(cpu, cpu->pc);
    push8(cpu, cpu->p);
    set_flag(cpu, FLAG_I, true);
    cpu->pc = cpu_read(0xFFFA) | (cpu_read(0xFFFB) << 8);
    cpu->cycles += 7;
    
    if (cpu->debug_mode) {
        printf("NMI triggered, jumping to $%04X\n", cpu->pc);
    }
}

// IRQ interrupt
void cpu_irq(CPU6502 *cpu) {
    if (!get_flag(cpu, FLAG_I)) {
        push16(cpu, cpu->pc);
        push8(cpu, cpu->p);
        set_flag(cpu, FLAG_I, true);
        cpu->pc = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8);
        cpu->cycles += 7;
        
        if (cpu->debug_mode) {
            printf("IRQ triggered, jumping to $%04X\n", cpu->pc);
        }
    }
}