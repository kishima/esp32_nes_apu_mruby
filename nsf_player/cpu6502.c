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
            // Always show APU access regardless of debug mode
            printf("*** APU ACCESS *** Read $%04X\n", addr);
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
        // Always show APU access regardless of debug mode
        printf("*** APU ACCESS *** Write $%04X = $%02X\n", addr, value);
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
    cpu->jammed = false;
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
    cpu->jammed = false;
    
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
    printf("PC:%04X A:%02X X:%02X Y:%02X SP:%02X P:%02X [%c%c%c%c%c%c%c] CYC:%d",
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

// Get description for opcode and operands
static void print_opcode_description(CPU6502 *cpu, uint8_t opcode, uint16_t original_pc) {
    uint16_t addr;
    uint8_t operand1, operand2;
    
    // Temporarily restore PC to read operands
    uint16_t saved_pc = cpu->pc;
    cpu->pc = original_pc + 1;
    
    printf(" - ");
    
    switch(opcode) {
        // Memory operations
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1: case 0xB1:
            printf("Load accumulator");
            break;
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE:
            printf("Load X register");
            break;
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC:
            printf("Load Y register");
            break;
        case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99: case 0x81: case 0x91:
            printf("Store accumulator");
            break;
        case 0x86: case 0x96: case 0x8E:
            printf("Store X register");
            break;
        case 0x84: case 0x94: case 0x8C:
            printf("Store Y register");
            break;
            
        // Arithmetic
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79: case 0x61: case 0x71:
            printf("Add with carry");
            break;
        case 0xE9: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9: case 0xE1: case 0xF1:
            printf("Subtract with carry");
            break;
        case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39: case 0x21: case 0x31:
            printf("Logical AND");
            break;
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19: case 0x01: case 0x11:
            printf("Logical OR");
            break;
        case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D: case 0x59: case 0x41: case 0x51:
            printf("Exclusive OR");
            break;
            
        // Stack operations
        case 0x48: printf("Push accumulator to stack"); break;
        case 0x68: printf("Pull accumulator from stack"); break;
        case 0x08: printf("Push processor status to stack"); break;
        case 0x28: printf("Pull processor status from stack"); break;
        
        // Jumps and calls
        case 0x4C: case 0x6C:
            operand1 = cpu_read(cpu->pc++);
            operand2 = cpu_read(cpu->pc++);
            addr = operand1 | (operand2 << 8);
            if (opcode == 0x6C) {
                printf("Jump indirect to $%04X", addr);
            } else {
                printf("Jump to $%04X", addr);
            }
            break;
        case 0x20:
            operand1 = cpu_read(cpu->pc++);
            operand2 = cpu_read(cpu->pc++);
            addr = operand1 | (operand2 << 8);
            printf("Call subroutine at $%04X", addr);
            break;
        case 0x60: printf("Return from subroutine"); break;
        case 0x40: printf("Return from interrupt"); break;
        
        // Transfers
        case 0xAA: printf("Transfer A to X"); break;
        case 0x8A: printf("Transfer X to A"); break;
        case 0xA8: printf("Transfer A to Y"); break;
        case 0x98: printf("Transfer Y to A"); break;
        case 0x9A: printf("Transfer X to stack pointer"); break;
        case 0xBA: printf("Transfer stack pointer to X"); break;
        
        // Increments/Decrements
        case 0xE8: printf("Increment X"); break;
        case 0xC8: printf("Increment Y"); break;
        case 0xCA: printf("Decrement X"); break;
        case 0x88: printf("Decrement Y"); break;
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: printf("Increment memory"); break;
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: printf("Decrement memory"); break;
        
        // Branches
        case 0x10: printf("Branch if plus"); break;
        case 0x30: printf("Branch if minus"); break;
        case 0x50: printf("Branch if overflow clear"); break;
        case 0x70: printf("Branch if overflow set"); break;
        case 0x90: printf("Branch if carry clear"); break;
        case 0xB0: printf("Branch if carry set"); break;
        case 0xD0: printf("Branch if not equal"); break;
        case 0xF0: printf("Branch if equal"); break;
        
        // Flag operations
        case 0x18: printf("Clear carry flag"); break;
        case 0x38: printf("Set carry flag"); break;
        case 0x58: printf("Clear interrupt disable"); break;
        case 0x78: printf("Set interrupt disable"); break;
        case 0xB8: printf("Clear overflow flag"); break;
        case 0xD8: printf("Clear decimal mode"); break;
        case 0xF8: printf("Set decimal mode"); break;
        
        // Comparisons
        case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD: case 0xD9: case 0xC1: case 0xD1:
            printf("Compare with accumulator");
            break;
        case 0xE0: case 0xE4: case 0xEC:
            printf("Compare with X register");
            break;
        case 0xC0: case 0xC4: case 0xCC:
            printf("Compare with Y register");
            break;
        
        // Shifts/Rotates
        case 0x0A: case 0x06: case 0x16: case 0x0E: case 0x1E: printf("Arithmetic shift left"); break;
        case 0x4A: case 0x46: case 0x56: case 0x4E: case 0x5E: printf("Logical shift right"); break;
        case 0x2A: case 0x26: case 0x36: case 0x2E: case 0x3E: printf("Rotate left"); break;
        case 0x6A: case 0x66: case 0x76: case 0x6E: case 0x7E: printf("Rotate right"); break;
        
        // Special
        case 0x00: printf("Break (software interrupt)"); break;
        case 0xEA: printf("No operation"); break;
        case 0x24: case 0x2C: printf("Bit test"); break;
        
        default:
            if ((opcode & 0x0F) == 0x02 || (opcode & 0x0F) == 0x12) {
                printf("Illegal instruction - CPU jam");
            } else {
                printf("Illegal/undocumented instruction");
            }
            break;
    }
    
    // Restore PC
    cpu->pc = saved_pc;
    printf("\n");
}

// Helper macros for addressing modes
#define IMMEDIATE() (cpu_read(cpu->pc++))
#define ZERO_PAGE() (cpu_read(cpu->pc++))
#define ZERO_PAGE_X() ((cpu_read(cpu->pc++) + cpu->x) & 0xFF)
#define ZERO_PAGE_Y() ((cpu_read(cpu->pc++) + cpu->y) & 0xFF)
#define ABSOLUTE() (cpu_read(cpu->pc++) | (cpu_read(cpu->pc++) << 8))
#define ABSOLUTE_X() (ABSOLUTE() + cpu->x)
#define ABSOLUTE_Y() (ABSOLUTE() + cpu->y)
#define INDIRECT_X() ({ uint8_t base = (cpu_read(cpu->pc++) + cpu->x) & 0xFF; \
                       cpu_read(base) | (cpu_read((base + 1) & 0xFF) << 8); })
#define INDIRECT_Y() ({ uint8_t base = cpu_read(cpu->pc++); \
                       (cpu_read(base) | (cpu_read((base + 1) & 0xFF) << 8)) + cpu->y; })

// ALU operations
static void adc(CPU6502 *cpu, uint8_t value) {
    uint16_t result = cpu->a + value + (get_flag(cpu, FLAG_C) ? 1 : 0);
    set_flag(cpu, FLAG_C, result > 0xFF);
    set_flag(cpu, FLAG_V, ((cpu->a ^ result) & (value ^ result) & 0x80) != 0);
    cpu->a = result & 0xFF;
    update_nz(cpu, cpu->a);
}

static void sbc(CPU6502 *cpu, uint8_t value) {
    adc(cpu, value ^ 0xFF);
}

static void cmp(CPU6502 *cpu, uint8_t reg, uint8_t value) {
    uint16_t result = reg - value;
    set_flag(cpu, FLAG_C, reg >= value);
    update_nz(cpu, result & 0xFF);
}

static void and_op(CPU6502 *cpu, uint8_t value) {
    cpu->a &= value;
    update_nz(cpu, cpu->a);
}

static void ora(CPU6502 *cpu, uint8_t value) {
    cpu->a |= value;
    update_nz(cpu, cpu->a);
}

static void eor(CPU6502 *cpu, uint8_t value) {
    cpu->a ^= value;
    update_nz(cpu, cpu->a);
}

static void bit(CPU6502 *cpu, uint8_t value) {
    set_flag(cpu, FLAG_Z, (cpu->a & value) == 0);
    set_flag(cpu, FLAG_V, (value & 0x40) != 0);
    set_flag(cpu, FLAG_N, (value & 0x80) != 0);
}

static uint8_t asl(CPU6502 *cpu, uint8_t value) {
    set_flag(cpu, FLAG_C, (value & 0x80) != 0);
    value <<= 1;
    update_nz(cpu, value);
    return value;
}

static uint8_t lsr(CPU6502 *cpu, uint8_t value) {
    set_flag(cpu, FLAG_C, (value & 0x01) != 0);
    value >>= 1;
    update_nz(cpu, value);
    return value;
}

static uint8_t rol(CPU6502 *cpu, uint8_t value) {
    bool old_carry = get_flag(cpu, FLAG_C);
    set_flag(cpu, FLAG_C, (value & 0x80) != 0);
    value = (value << 1) | (old_carry ? 1 : 0);
    update_nz(cpu, value);
    return value;
}

static uint8_t ror(CPU6502 *cpu, uint8_t value) {
    bool old_carry = get_flag(cpu, FLAG_C);
    set_flag(cpu, FLAG_C, (value & 0x01) != 0);
    value = (value >> 1) | (old_carry ? 0x80 : 0);
    update_nz(cpu, value);
    return value;
}

static uint8_t inc(CPU6502 *cpu, uint8_t value) {
    value++;
    update_nz(cpu, value);
    return value;
}

static uint8_t dec(CPU6502 *cpu, uint8_t value) {
    value--;
    update_nz(cpu, value);
    return value;
}

// Branch helper
static void branch(CPU6502 *cpu, bool condition) {
    int8_t offset = (int8_t)IMMEDIATE();
    if (condition) {
        uint16_t old_pc = cpu->pc;
        cpu->pc += offset;
        cpu->cycles += ((old_pc & 0xFF00) != (cpu->pc & 0xFF00)) ? 2 : 1;
    }
}

// Execute one instruction
void cpu_step(CPU6502 *cpu) {
    uint8_t opcode = cpu_read(cpu->pc);
    uint16_t addr;
    uint8_t value;
    
    uint16_t original_pc = cpu->pc;
    
    if (cpu->debug_mode) {
        printf("$%04X: %02X (%s) ", cpu->pc, opcode, opcode_names[opcode]);
        cpu_print_state(cpu);
        print_opcode_description(cpu, opcode, original_pc);
    }
    
    cpu->pc++;
    
    // Complete 6502 opcode implementation (all 256 opcodes)
    switch(opcode) {
        // 0x00-0x0F
        case 0x00: // BRK
            push16(cpu, cpu->pc);
            push8(cpu, cpu->p | FLAG_B);
            set_flag(cpu, FLAG_I, true);
            cpu->pc = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8);
            cpu->cycles += 7;
            break;
        case 0x01: // ORA ($nn,X)
            ora(cpu, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52: case 0x62: case 0x72:
        case 0x92: case 0xB2: case 0xD2: case 0xF2: // JAM (illegal)
            printf("CPU jammed at $%04X\n", cpu->pc - 1);
            cpu->pc--; // Stay at the same instruction
            cpu->jammed = true;
            cpu->cycles += 2;
            break;
        case 0x03: // SLO ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x04: case 0x44: case 0x64: // NOP $nn (illegal)
            cpu->pc++;
            cpu->cycles += 3;
            break;
        case 0x05: // ORA $nn
            ora(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0x06: // ASL $nn
            addr = ZERO_PAGE();
            cpu_write(addr, asl(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0x07: // SLO $nn (illegal)
            addr = ZERO_PAGE();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 5;
            break;
        case 0x08: // PHP
            push8(cpu, cpu->p | FLAG_B);
            cpu->cycles += 3;
            break;
        case 0x09: // ORA #$nn
            ora(cpu, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0x0A: // ASL A
            cpu->a = asl(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x0B: // ANC #$nn (illegal)
            and_op(cpu, IMMEDIATE());
            set_flag(cpu, FLAG_C, get_flag(cpu, FLAG_N));
            cpu->cycles += 2;
            break;
        case 0x0C: // NOP $nnnn (illegal)
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x0D: // ORA $nnnn
            ora(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0x0E: // ASL $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, asl(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x0F: // SLO $nnnn (illegal)
            addr = ABSOLUTE();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 6;
            break;
            
        // 0x10-0x1F
        case 0x10: // BPL
            branch(cpu, !get_flag(cpu, FLAG_N));
            cpu->cycles += 2;
            break;
        case 0x11: // ORA ($nn),Y
            ora(cpu, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0x13: // SLO ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: // NOP $nn,X (illegal)
            cpu->pc++;
            cpu->cycles += 4;
            break;
        case 0x15: // ORA $nn,X
            ora(cpu, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0x16: // ASL $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, asl(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x17: // SLO $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 6;
            break;
        case 0x18: // CLC
            set_flag(cpu, FLAG_C, false);
            cpu->cycles += 2;
            break;
        case 0x19: // ORA $nnnn,Y
            ora(cpu, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: // NOP (illegal)
            cpu->cycles += 2;
            break;
        case 0x1B: // SLO $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 7;
            break;
        case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: // NOP $nnnn,X (illegal)
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x1D: // ORA $nnnn,X
            ora(cpu, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0x1E: // ASL $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, asl(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0x1F: // SLO $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = asl(cpu, cpu_read(addr));
            cpu_write(addr, value);
            ora(cpu, value);
            cpu->cycles += 7;
            break;
            
        // 0x20-0x2F
        case 0x20: // JSR $nnnn
            addr = ABSOLUTE();
            push16(cpu, cpu->pc - 1);
            cpu->pc = addr;
            cpu->cycles += 6;
            break;
        case 0x21: // AND ($nn,X)
            and_op(cpu, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0x23: // RLA ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x24: // BIT $nn
            bit(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0x25: // AND $nn
            and_op(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0x26: // ROL $nn
            addr = ZERO_PAGE();
            cpu_write(addr, rol(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0x27: // RLA $nn (illegal)
            addr = ZERO_PAGE();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 5;
            break;
        case 0x28: // PLP
            cpu->p = pull8(cpu);
            cpu->cycles += 4;
            break;
        case 0x29: // AND #$nn
            and_op(cpu, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0x2A: // ROL A
            cpu->a = rol(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x2B: // ANC #$nn (illegal)
            and_op(cpu, IMMEDIATE());
            set_flag(cpu, FLAG_C, get_flag(cpu, FLAG_N));
            cpu->cycles += 2;
            break;
        case 0x2C: // BIT $nnnn
            bit(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0x2D: // AND $nnnn
            and_op(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0x2E: // ROL $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, rol(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x2F: // RLA $nnnn (illegal)
            addr = ABSOLUTE();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 6;
            break;
            
        // 0x30-0x3F
        case 0x30: // BMI
            branch(cpu, get_flag(cpu, FLAG_N));
            cpu->cycles += 2;
            break;
        case 0x31: // AND ($nn),Y
            and_op(cpu, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0x33: // RLA ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x35: // AND $nn,X
            and_op(cpu, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0x36: // ROL $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, rol(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x37: // RLA $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 6;
            break;
        case 0x38: // SEC
            set_flag(cpu, FLAG_C, true);
            cpu->cycles += 2;
            break;
        case 0x39: // AND $nnnn,Y
            and_op(cpu, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0x3B: // RLA $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 7;
            break;
        case 0x3D: // AND $nnnn,X
            and_op(cpu, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0x3E: // ROL $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, rol(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0x3F: // RLA $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = rol(cpu, cpu_read(addr));
            cpu_write(addr, value);
            and_op(cpu, value);
            cpu->cycles += 7;
            break;
            
        // 0x40-0x4F
        case 0x40: // RTI
            cpu->p = pull8(cpu) & ~FLAG_B;
            cpu->pc = pull16(cpu);
            cpu->cycles += 6;
            break;
        case 0x41: // EOR ($nn,X)
            eor(cpu, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0x43: // SRE ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x45: // EOR $nn
            eor(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0x46: // LSR $nn
            addr = ZERO_PAGE();
            cpu_write(addr, lsr(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0x47: // SRE $nn (illegal)
            addr = ZERO_PAGE();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 5;
            break;
        case 0x48: // PHA
            push8(cpu, cpu->a);
            cpu->cycles += 3;
            break;
        case 0x49: // EOR #$nn
            eor(cpu, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0x4A: // LSR A
            cpu->a = lsr(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x4B: // ALR #$nn (illegal)
            and_op(cpu, IMMEDIATE());
            cpu->a = lsr(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x4C: // JMP $nnnn
            cpu->pc = ABSOLUTE();
            cpu->cycles += 3;
            break;
        case 0x4D: // EOR $nnnn
            eor(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0x4E: // LSR $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, lsr(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x4F: // SRE $nnnn (illegal)
            addr = ABSOLUTE();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 6;
            break;
            
        // 0x50-0x5F
        case 0x50: // BVC
            branch(cpu, !get_flag(cpu, FLAG_V));
            cpu->cycles += 2;
            break;
        case 0x51: // EOR ($nn),Y
            eor(cpu, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0x53: // SRE ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x55: // EOR $nn,X
            eor(cpu, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0x56: // LSR $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, lsr(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x57: // SRE $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 6;
            break;
        case 0x58: // CLI
            set_flag(cpu, FLAG_I, false);
            cpu->cycles += 2;
            break;
        case 0x59: // EOR $nnnn,Y
            eor(cpu, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0x5B: // SRE $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 7;
            break;
        case 0x5D: // EOR $nnnn,X
            eor(cpu, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0x5E: // LSR $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, lsr(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0x5F: // SRE $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = lsr(cpu, cpu_read(addr));
            cpu_write(addr, value);
            eor(cpu, value);
            cpu->cycles += 7;
            break;
            
        // 0x60-0x6F
        case 0x60: // RTS
            cpu->pc = pull16(cpu) + 1;
            cpu->cycles += 6;
            break;
        case 0x61: // ADC ($nn,X)
            adc(cpu, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0x63: // RRA ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x65: // ADC $nn
            adc(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0x66: // ROR $nn
            addr = ZERO_PAGE();
            cpu_write(addr, ror(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0x67: // RRA $nn (illegal)
            addr = ZERO_PAGE();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 5;
            break;
        case 0x68: // PLA
            cpu->a = pull8(cpu);
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
        case 0x69: // ADC #$nn
            adc(cpu, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0x6A: // ROR A
            cpu->a = ror(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x6B: // ARR #$nn (illegal)
            and_op(cpu, IMMEDIATE());
            cpu->a = ror(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x6C: // JMP ($nnnn)
            addr = ABSOLUTE();
            // Handle page boundary bug
            if ((addr & 0xFF) == 0xFF) {
                cpu->pc = cpu_read(addr) | (cpu_read(addr & 0xFF00) << 8);
            } else {
                cpu->pc = cpu_read(addr) | (cpu_read(addr + 1) << 8);
            }
            cpu->cycles += 5;
            break;
        case 0x6D: // ADC $nnnn
            adc(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0x6E: // ROR $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, ror(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x6F: // RRA $nnnn (illegal)
            addr = ABSOLUTE();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 6;
            break;
            
        // 0x70-0x7F
        case 0x70: // BVS
            branch(cpu, get_flag(cpu, FLAG_V));
            cpu->cycles += 2;
            break;
        case 0x71: // ADC ($nn),Y
            adc(cpu, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0x73: // RRA ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 8;
            break;
        case 0x75: // ADC $nn,X
            adc(cpu, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0x76: // ROR $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, ror(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0x77: // RRA $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 6;
            break;
        case 0x78: // SEI
            set_flag(cpu, FLAG_I, true);
            cpu->cycles += 2;
            break;
        case 0x79: // ADC $nnnn,Y
            adc(cpu, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0x7B: // RRA $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 7;
            break;
        case 0x7D: // ADC $nnnn,X
            adc(cpu, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0x7E: // ROR $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, ror(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0x7F: // RRA $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = ror(cpu, cpu_read(addr));
            cpu_write(addr, value);
            adc(cpu, value);
            cpu->cycles += 7;
            break;
            
        // 0x80-0x8F
        case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2: // NOP #$nn (illegal)
            cpu->pc++;
            cpu->cycles += 2;
            break;
        case 0x81: // STA ($nn,X)
            cpu_write(INDIRECT_X(), cpu->a);
            cpu->cycles += 6;
            break;
        case 0x83: // SAX ($nn,X) (illegal)
            cpu_write(INDIRECT_X(), cpu->a & cpu->x);
            cpu->cycles += 6;
            break;
        case 0x84: // STY $nn
            cpu_write(ZERO_PAGE(), cpu->y);
            cpu->cycles += 3;
            break;
        case 0x85: // STA $nn
            cpu_write(ZERO_PAGE(), cpu->a);
            cpu->cycles += 3;
            break;
        case 0x86: // STX $nn
            cpu_write(ZERO_PAGE(), cpu->x);
            cpu->cycles += 3;
            break;
        case 0x87: // SAX $nn (illegal)
            cpu_write(ZERO_PAGE(), cpu->a & cpu->x);
            cpu->cycles += 3;
            break;
        case 0x88: // DEY
            cpu->y = dec(cpu, cpu->y);
            cpu->cycles += 2;
            break;
        case 0x8A: // TXA
            cpu->a = cpu->x;
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x8B: // XAA #$nn (illegal)
            cpu->a = cpu->x & IMMEDIATE();
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x8C: // STY $nnnn
            cpu_write(ABSOLUTE(), cpu->y);
            cpu->cycles += 4;
            break;
        case 0x8D: // STA $nnnn
            cpu_write(ABSOLUTE(), cpu->a);
            cpu->cycles += 4;
            break;
        case 0x8E: // STX $nnnn
            cpu_write(ABSOLUTE(), cpu->x);
            cpu->cycles += 4;
            break;
        case 0x8F: // SAX $nnnn (illegal)
            cpu_write(ABSOLUTE(), cpu->a & cpu->x);
            cpu->cycles += 4;
            break;
            
        // 0x90-0x9F
        case 0x90: // BCC
            branch(cpu, !get_flag(cpu, FLAG_C));
            cpu->cycles += 2;
            break;
        case 0x91: // STA ($nn),Y
            cpu_write(INDIRECT_Y(), cpu->a);
            cpu->cycles += 6;
            break;
        case 0x93: // AHX ($nn),Y (illegal)
            cpu_write(INDIRECT_Y(), cpu->a & cpu->x & ((INDIRECT_Y() >> 8) + 1));
            cpu->cycles += 6;
            break;
        case 0x94: // STY $nn,X
            cpu_write(ZERO_PAGE_X(), cpu->y);
            cpu->cycles += 4;
            break;
        case 0x95: // STA $nn,X
            cpu_write(ZERO_PAGE_X(), cpu->a);
            cpu->cycles += 4;
            break;
        case 0x96: // STX $nn,Y
            cpu_write(ZERO_PAGE_Y(), cpu->x);
            cpu->cycles += 4;
            break;
        case 0x97: // SAX $nn,Y (illegal)
            cpu_write(ZERO_PAGE_Y(), cpu->a & cpu->x);
            cpu->cycles += 4;
            break;
        case 0x98: // TYA
            cpu->a = cpu->y;
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0x99: // STA $nnnn,Y
            cpu_write(ABSOLUTE_Y(), cpu->a);
            cpu->cycles += 5;
            break;
        case 0x9A: // TXS
            cpu->sp = cpu->x;
            cpu->cycles += 2;
            break;
        case 0x9B: // TAS $nnnn,Y (illegal)
            cpu->sp = cpu->a & cpu->x;
            addr = ABSOLUTE_Y();
            cpu_write(addr, cpu->sp & ((addr >> 8) + 1));
            cpu->cycles += 5;
            break;
        case 0x9C: // SHY $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            cpu_write(addr, cpu->y & ((addr >> 8) + 1));
            cpu->cycles += 5;
            break;
        case 0x9D: // STA $nnnn,X
            cpu_write(ABSOLUTE_X(), cpu->a);
            cpu->cycles += 5;
            break;
        case 0x9E: // SHX $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            cpu_write(addr, cpu->x & ((addr >> 8) + 1));
            cpu->cycles += 5;
            break;
        case 0x9F: // AHX $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            cpu_write(addr, cpu->a & cpu->x & ((addr >> 8) + 1));
            cpu->cycles += 5;
            break;
            
        // 0xA0-0xAF
        case 0xA0: // LDY #$nn
            cpu->y = IMMEDIATE();
            update_nz(cpu, cpu->y);
            cpu->cycles += 2;
            break;
        case 0xA1: // LDA ($nn,X)
            cpu->a = cpu_read(INDIRECT_X());
            update_nz(cpu, cpu->a);
            cpu->cycles += 6;
            break;
        case 0xA2: // LDX #$nn
            cpu->x = IMMEDIATE();
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xA3: // LAX ($nn,X) (illegal)
            value = cpu_read(INDIRECT_X());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 6;
            break;
        case 0xA4: // LDY $nn
            cpu->y = cpu_read(ZERO_PAGE());
            update_nz(cpu, cpu->y);
            cpu->cycles += 3;
            break;
        case 0xA5: // LDA $nn
            cpu->a = cpu_read(ZERO_PAGE());
            update_nz(cpu, cpu->a);
            cpu->cycles += 3;
            break;
        case 0xA6: // LDX $nn
            cpu->x = cpu_read(ZERO_PAGE());
            update_nz(cpu, cpu->x);
            cpu->cycles += 3;
            break;
        case 0xA7: // LAX $nn (illegal)
            value = cpu_read(ZERO_PAGE());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 3;
            break;
        case 0xA8: // TAY
            cpu->y = cpu->a;
            update_nz(cpu, cpu->y);
            cpu->cycles += 2;
            break;
        case 0xA9: // LDA #$nn
            cpu->a = IMMEDIATE();
            update_nz(cpu, cpu->a);
            cpu->cycles += 2;
            break;
        case 0xAA: // TAX
            cpu->x = cpu->a;
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xAB: // LAX #$nn (illegal)
            value = IMMEDIATE();
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 2;
            break;
        case 0xAC: // LDY $nnnn
            cpu->y = cpu_read(ABSOLUTE());
            update_nz(cpu, cpu->y);
            cpu->cycles += 4;
            break;
        case 0xAD: // LDA $nnnn
            cpu->a = cpu_read(ABSOLUTE());
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
        case 0xAE: // LDX $nnnn
            cpu->x = cpu_read(ABSOLUTE());
            update_nz(cpu, cpu->x);
            cpu->cycles += 4;
            break;
        case 0xAF: // LAX $nnnn (illegal)
            value = cpu_read(ABSOLUTE());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 4;
            break;
            
        // 0xB0-0xBF
        case 0xB0: // BCS
            branch(cpu, get_flag(cpu, FLAG_C));
            cpu->cycles += 2;
            break;
        case 0xB1: // LDA ($nn),Y
            cpu->a = cpu_read(INDIRECT_Y());
            update_nz(cpu, cpu->a);
            cpu->cycles += 5;
            break;
        case 0xB3: // LAX ($nn),Y (illegal)
            value = cpu_read(INDIRECT_Y());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 5;
            break;
        case 0xB4: // LDY $nn,X
            cpu->y = cpu_read(ZERO_PAGE_X());
            update_nz(cpu, cpu->y);
            cpu->cycles += 4;
            break;
        case 0xB5: // LDA $nn,X
            cpu->a = cpu_read(ZERO_PAGE_X());
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
        case 0xB6: // LDX $nn,Y
            cpu->x = cpu_read(ZERO_PAGE_Y());
            update_nz(cpu, cpu->x);
            cpu->cycles += 4;
            break;
        case 0xB7: // LAX $nn,Y (illegal)
            value = cpu_read(ZERO_PAGE_Y());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 4;
            break;
        case 0xB8: // CLV
            set_flag(cpu, FLAG_V, false);
            cpu->cycles += 2;
            break;
        case 0xB9: // LDA $nnnn,Y
            cpu->a = cpu_read(ABSOLUTE_Y());
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
        case 0xBA: // TSX
            cpu->x = cpu->sp;
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xBB: // LAS $nnnn,Y (illegal)
            value = cpu_read(ABSOLUTE_Y()) & cpu->sp;
            cpu->a = cpu->x = cpu->sp = value;
            update_nz(cpu, value);
            cpu->cycles += 4;
            break;
        case 0xBC: // LDY $nnnn,X
            cpu->y = cpu_read(ABSOLUTE_X());
            update_nz(cpu, cpu->y);
            cpu->cycles += 4;
            break;
        case 0xBD: // LDA $nnnn,X
            cpu->a = cpu_read(ABSOLUTE_X());
            update_nz(cpu, cpu->a);
            cpu->cycles += 4;
            break;
        case 0xBE: // LDX $nnnn,Y
            cpu->x = cpu_read(ABSOLUTE_Y());
            update_nz(cpu, cpu->x);
            cpu->cycles += 4;
            break;
        case 0xBF: // LAX $nnnn,Y (illegal)
            value = cpu_read(ABSOLUTE_Y());
            cpu->a = cpu->x = value;
            update_nz(cpu, value);
            cpu->cycles += 4;
            break;
            
        // 0xC0-0xCF
        case 0xC0: // CPY #$nn
            cmp(cpu, cpu->y, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0xC1: // CMP ($nn,X)
            cmp(cpu, cpu->a, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0xC3: // DCP ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 8;
            break;
        case 0xC4: // CPY $nn
            cmp(cpu, cpu->y, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0xC5: // CMP $nn
            cmp(cpu, cpu->a, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0xC6: // DEC $nn
            addr = ZERO_PAGE();
            cpu_write(addr, dec(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0xC7: // DCP $nn (illegal)
            addr = ZERO_PAGE();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 5;
            break;
        case 0xC8: // INY
            cpu->y = inc(cpu, cpu->y);
            cpu->cycles += 2;
            break;
        case 0xC9: // CMP #$nn
            cmp(cpu, cpu->a, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0xCA: // DEX
            cpu->x = dec(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xCB: // AXS #$nn (illegal)
            value = IMMEDIATE();
            cpu->x = (cpu->a & cpu->x) - value;
            set_flag(cpu, FLAG_C, (cpu->a & cpu->x) >= value);
            update_nz(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xCC: // CPY $nnnn
            cmp(cpu, cpu->y, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0xCD: // CMP $nnnn
            cmp(cpu, cpu->a, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0xCE: // DEC $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, dec(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0xCF: // DCP $nnnn (illegal)
            addr = ABSOLUTE();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 6;
            break;
            
        // 0xD0-0xDF
        case 0xD0: // BNE
            branch(cpu, !get_flag(cpu, FLAG_Z));
            cpu->cycles += 2;
            break;
        case 0xD1: // CMP ($nn),Y
            cmp(cpu, cpu->a, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0xD3: // DCP ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 8;
            break;
        case 0xD5: // CMP $nn,X
            cmp(cpu, cpu->a, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0xD6: // DEC $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, dec(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0xD7: // DCP $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 6;
            break;
        case 0xD8: // CLD
            set_flag(cpu, FLAG_D, false);
            cpu->cycles += 2;
            break;
        case 0xD9: // CMP $nnnn,Y
            cmp(cpu, cpu->a, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0xDB: // DCP $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 7;
            break;
        case 0xDD: // CMP $nnnn,X
            cmp(cpu, cpu->a, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0xDE: // DEC $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, dec(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0xDF: // DCP $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = dec(cpu, cpu_read(addr));
            cpu_write(addr, value);
            cmp(cpu, cpu->a, value);
            cpu->cycles += 7;
            break;
            
        // 0xE0-0xEF
        case 0xE0: // CPX #$nn
            cmp(cpu, cpu->x, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0xE1: // SBC ($nn,X)
            sbc(cpu, cpu_read(INDIRECT_X()));
            cpu->cycles += 6;
            break;
        case 0xE3: // ISB ($nn,X) (illegal)
            addr = INDIRECT_X();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 8;
            break;
        case 0xE4: // CPX $nn
            cmp(cpu, cpu->x, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0xE5: // SBC $nn
            sbc(cpu, cpu_read(ZERO_PAGE()));
            cpu->cycles += 3;
            break;
        case 0xE6: // INC $nn
            addr = ZERO_PAGE();
            cpu_write(addr, inc(cpu, cpu_read(addr)));
            cpu->cycles += 5;
            break;
        case 0xE7: // ISB $nn (illegal)
            addr = ZERO_PAGE();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 5;
            break;
        case 0xE8: // INX
            cpu->x = inc(cpu, cpu->x);
            cpu->cycles += 2;
            break;
        case 0xE9: case 0xEB: // SBC #$nn
            sbc(cpu, IMMEDIATE());
            cpu->cycles += 2;
            break;
        case 0xEA: // NOP
            cpu->cycles += 2;
            break;
        case 0xEC: // CPX $nnnn
            cmp(cpu, cpu->x, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0xED: // SBC $nnnn
            sbc(cpu, cpu_read(ABSOLUTE()));
            cpu->cycles += 4;
            break;
        case 0xEE: // INC $nnnn
            addr = ABSOLUTE();
            cpu_write(addr, inc(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0xEF: // ISB $nnnn (illegal)
            addr = ABSOLUTE();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 6;
            break;
            
        // 0xF0-0xFF
        case 0xF0: // BEQ
            branch(cpu, get_flag(cpu, FLAG_Z));
            cpu->cycles += 2;
            break;
        case 0xF1: // SBC ($nn),Y
            sbc(cpu, cpu_read(INDIRECT_Y()));
            cpu->cycles += 5;
            break;
        case 0xF3: // ISB ($nn),Y (illegal)
            addr = INDIRECT_Y();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 8;
            break;
        case 0xF5: // SBC $nn,X
            sbc(cpu, cpu_read(ZERO_PAGE_X()));
            cpu->cycles += 4;
            break;
        case 0xF6: // INC $nn,X
            addr = ZERO_PAGE_X();
            cpu_write(addr, inc(cpu, cpu_read(addr)));
            cpu->cycles += 6;
            break;
        case 0xF7: // ISB $nn,X (illegal)
            addr = ZERO_PAGE_X();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 6;
            break;
        case 0xF8: // SED
            set_flag(cpu, FLAG_D, true);
            cpu->cycles += 2;
            break;
        case 0xF9: // SBC $nnnn,Y
            sbc(cpu, cpu_read(ABSOLUTE_Y()));
            cpu->cycles += 4;
            break;
        case 0xFB: // ISB $nnnn,Y (illegal)
            addr = ABSOLUTE_Y();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 7;
            break;
        case 0xFD: // SBC $nnnn,X
            sbc(cpu, cpu_read(ABSOLUTE_X()));
            cpu->cycles += 4;
            break;
        case 0xFE: // INC $nnnn,X
            addr = ABSOLUTE_X();
            cpu_write(addr, inc(cpu, cpu_read(addr)));
            cpu->cycles += 7;
            break;
        case 0xFF: // ISB $nnnn,X (illegal)
            addr = ABSOLUTE_X();
            value = inc(cpu, cpu_read(addr));
            cpu_write(addr, value);
            sbc(cpu, value);
            cpu->cycles += 7;
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