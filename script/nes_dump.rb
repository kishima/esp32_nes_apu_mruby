#!/usr/bin/env ruby
# NES ROM Dumper and 6502 Disassembler
# Parses NES ROM files and displays machine code with assembly disassembly

# メモリ空間の概要
# $0000-$07FF  内蔵RAM（2KB）
# $0800-$1FFF  内蔵RAMのミラー
# $2000-$2007  PPUレジスタ
# $2008-$3FFF  PPUレジスタのミラー
# $4000-$4017  APU / I/Oレジスタ
# $4018-$401F  テスト用領域
# $4020-$5FFF  カートリッジ拡張領域（拡張RAMなど）
# $6000-$7FFF  セーブRAM（バッテリーバックアップRAMなど）
# $8000-$FFFF  PRG-ROM（プログラムコード）


# Register Info
#  NESの一般的なレジスタを体系的に列挙します。

#   6502 CPU レジスタ

#   A (Accumulator)    - 8bit メイン演算レジスタ
#   X (Index X)        - 8bit インデックスレジスタ
#   Y (Index Y)        - 8bit インデックスレジスタ
#   S (Stack Pointer)  - 8bit スタックポインタ
#   PC (Program Counter) - 16bit プログラムカウンタ
#   P (Status)         - 8bit フラグレジスタ
#   (N,V,-,B,D,I,Z,C)

#   PPU (Picture Processing Unit) レジスタ

#   $2000 PPUCTRL     - PPU制御レジスタ1
#   (NMI有効、マスター/スレーブ、スプライト高さ等)
#   $2001 PPUMASK     - PPU制御レジスタ2
#   (カラー強調、スプライト/背景表示等)
#   $2002 PPUSTATUS   - PPU状態レジスタ
#   (VBlank、スプライト0ヒット、オーバーフロー)
#   $2003 OAMADDR     - OAMアドレスレジスタ
#   $2004 OAMDATA     - OAMデータレジスタ
#   $2005 PPUSCROLL   - スクロールレジスタ
#   $2006 PPUADDR     - PPUアドレスレジスタ
#   $2007 PPUDATA     - PPUデータレジスタ

#   APU (Audio Processing Unit) レジスタ

#   # Pulse波 1 (矩形波チャンネル1)
#   $4000 SQ1_VOL     - 音量、Duty、エンベロープ制御
#   $4001 SQ1_SWEEP   - スイープ制御
#   $4002 SQ1_LO      - 周波数下位8ビット
#   $4003 SQ1_HI      - 周波数上位3ビット +
#   長さカウンタ

#   # Pulse波 2 (矩形波チャンネル2)
#   $4004 SQ2_VOL     - 音量、Duty、エンベロープ制御
#   $4005 SQ2_SWEEP   - スイープ制御
#   $4006 SQ2_LO      - 周波数下位8ビット
#   $4007 SQ2_HI      - 周波数上位3ビット +
#   長さカウンタ

#   # Triangle波 (三角波チャンネル)
#   $4008 TRI_LINEAR  - 線形カウンタ制御
#   $4009            - (未使用)
#   $400A TRI_LO      - 周波数下位8ビット
#   $400B TRI_HI      - 周波数上位3ビット + 
#   長さカウンタ

#   # Noise (ノイズチャンネル)
#   $400C NOISE_VOL   - 音量、エンベロープ制御
#   $400D            - (未使用)
#   $400E NOISE_LO    - 周期、モード制御
#   $400F NOISE_HI    - 長さカウンタ

#   # DMC (Delta Modulation Channel)
#   $4010 DMC_FREQ    - 周波数、ループ制御
#   $4011 DMC_RAW     - 7ビット直接出力
#   $4012 DMC_START   - サンプル開始アドレス
#   $4013 DMC_LEN     - サンプル長

#   # APU制御
#   $4015 SND_CHN     - 
#   チャンネル有効/無効、長さカウンタ状態
#   $4017 JOY2        - 
#   フレームカウンタ、コントローラ2 (APU制御も兼用)

#   入力レジスタ

#   $4016 JOY1        - コントローラ1データ
#   $4017 JOY2        - コントローラ2データ +
#   フレームカウンタ

#   その他

#   $4014 OAMDMA      - DMA転送レジスタ
#   (スプライト高速転送)

#   メモリマップ領域

#   $0000-$07FF       - 内蔵RAM (2KB、ミラー含む)
#   $2000-$3FFF       - PPUレジスタ
#   (8バイトが1KB毎にミラー)
#   $4000-$401F       - APU/入力レジスタ
#   $4020-$FFFF       - カートリッジ空間 (PRG-ROM/RAM)

  require 'optparse'

class NES6502Disassembler
  # 6502 addressing modes
  ADDRESSING_MODES = {
    :implicit => 1,
    :accumulator => 1,
    :immediate => 2,
    :zero_page => 2,
    :zero_page_x => 2,
    :zero_page_y => 2,
    :relative => 2,
    :absolute => 3,
    :absolute_x => 3,
    :absolute_y => 3,
    :indirect => 3,
    :indexed_indirect => 2,  # (zp,X)
    :indirect_indexed => 2   # (zp),Y
  }

  # 6502 instruction set with opcodes, mnemonics, addressing modes, and descriptions
  INSTRUCTIONS = {
    # ADC - Add with Carry
    0x69 => { mnemonic: "ADC", mode: :immediate, desc: "Add immediate value to accumulator with carry" },
    0x65 => { mnemonic: "ADC", mode: :zero_page, desc: "Add zero page value to accumulator with carry" },
    0x75 => { mnemonic: "ADC", mode: :zero_page_x, desc: "Add zero page,X value to accumulator with carry" },
    0x6D => { mnemonic: "ADC", mode: :absolute, desc: "Add absolute value to accumulator with carry" },
    0x7D => { mnemonic: "ADC", mode: :absolute_x, desc: "Add absolute,X value to accumulator with carry" },
    0x79 => { mnemonic: "ADC", mode: :absolute_y, desc: "Add absolute,Y value to accumulator with carry" },
    0x61 => { mnemonic: "ADC", mode: :indexed_indirect, desc: "Add (zero page,X) value to accumulator with carry" },
    0x71 => { mnemonic: "ADC", mode: :indirect_indexed, desc: "Add (zero page),Y value to accumulator with carry" },

    # AND - Logical AND
    0x29 => { mnemonic: "AND", mode: :immediate, desc: "AND immediate value with accumulator" },
    0x25 => { mnemonic: "AND", mode: :zero_page, desc: "AND zero page value with accumulator" },
    0x35 => { mnemonic: "AND", mode: :zero_page_x, desc: "AND zero page,X value with accumulator" },
    0x2D => { mnemonic: "AND", mode: :absolute, desc: "AND absolute value with accumulator" },
    0x3D => { mnemonic: "AND", mode: :absolute_x, desc: "AND absolute,X value with accumulator" },
    0x39 => { mnemonic: "AND", mode: :absolute_y, desc: "AND absolute,Y value with accumulator" },
    0x21 => { mnemonic: "AND", mode: :indexed_indirect, desc: "AND (zero page,X) value with accumulator" },
    0x31 => { mnemonic: "AND", mode: :indirect_indexed, desc: "AND (zero page),Y value with accumulator" },

    # ASL - Arithmetic Shift Left
    0x0A => { mnemonic: "ASL", mode: :accumulator, desc: "Shift accumulator left one bit" },
    0x06 => { mnemonic: "ASL", mode: :zero_page, desc: "Shift zero page value left one bit" },
    0x16 => { mnemonic: "ASL", mode: :zero_page_x, desc: "Shift zero page,X value left one bit" },
    0x0E => { mnemonic: "ASL", mode: :absolute, desc: "Shift absolute value left one bit" },
    0x1E => { mnemonic: "ASL", mode: :absolute_x, desc: "Shift absolute,X value left one bit" },

    # Branch Instructions
    0x90 => { mnemonic: "BCC", mode: :relative, desc: "Branch if carry clear" },
    0xB0 => { mnemonic: "BCS", mode: :relative, desc: "Branch if carry set" },
    0xF0 => { mnemonic: "BEQ", mode: :relative, desc: "Branch if equal (zero flag set)" },
    0x30 => { mnemonic: "BMI", mode: :relative, desc: "Branch if minus (negative flag set)" },
    0xD0 => { mnemonic: "BNE", mode: :relative, desc: "Branch if not equal (zero flag clear)" },
    0x10 => { mnemonic: "BPL", mode: :relative, desc: "Branch if plus (negative flag clear)" },
    0x50 => { mnemonic: "BVC", mode: :relative, desc: "Branch if overflow clear" },
    0x70 => { mnemonic: "BVS", mode: :relative, desc: "Branch if overflow set" },

    # BIT - Bit Test
    0x24 => { mnemonic: "BIT", mode: :zero_page, desc: "Test bits in accumulator with zero page value" },
    0x2C => { mnemonic: "BIT", mode: :absolute, desc: "Test bits in accumulator with absolute value" },

    # BRK - Break
    0x00 => { mnemonic: "BRK", mode: :implicit, desc: "Force interrupt (software break)" },

    # Clear/Set Instructions
    0x18 => { mnemonic: "CLC", mode: :implicit, desc: "Clear carry flag" },
    0xD8 => { mnemonic: "CLD", mode: :implicit, desc: "Clear decimal flag" },
    0x58 => { mnemonic: "CLI", mode: :implicit, desc: "Clear interrupt disable flag" },
    0xB8 => { mnemonic: "CLV", mode: :implicit, desc: "Clear overflow flag" },
    0x38 => { mnemonic: "SEC", mode: :implicit, desc: "Set carry flag" },
    0xF8 => { mnemonic: "SED", mode: :implicit, desc: "Set decimal flag" },
    0x78 => { mnemonic: "SEI", mode: :implicit, desc: "Set interrupt disable flag" },

    # CMP - Compare Accumulator
    0xC9 => { mnemonic: "CMP", mode: :immediate, desc: "Compare accumulator with immediate value" },
    0xC5 => { mnemonic: "CMP", mode: :zero_page, desc: "Compare accumulator with zero page value" },
    0xD5 => { mnemonic: "CMP", mode: :zero_page_x, desc: "Compare accumulator with zero page,X value" },
    0xCD => { mnemonic: "CMP", mode: :absolute, desc: "Compare accumulator with absolute value" },
    0xDD => { mnemonic: "CMP", mode: :absolute_x, desc: "Compare accumulator with absolute,X value" },
    0xD9 => { mnemonic: "CMP", mode: :absolute_y, desc: "Compare accumulator with absolute,Y value" },
    0xC1 => { mnemonic: "CMP", mode: :indexed_indirect, desc: "Compare accumulator with (zero page,X) value" },
    0xD1 => { mnemonic: "CMP", mode: :indirect_indexed, desc: "Compare accumulator with (zero page),Y value" },

    # CPX - Compare X Register
    0xE0 => { mnemonic: "CPX", mode: :immediate, desc: "Compare X register with immediate value" },
    0xE4 => { mnemonic: "CPX", mode: :zero_page, desc: "Compare X register with zero page value" },
    0xEC => { mnemonic: "CPX", mode: :absolute, desc: "Compare X register with absolute value" },

    # CPY - Compare Y Register
    0xC0 => { mnemonic: "CPY", mode: :immediate, desc: "Compare Y register with immediate value" },
    0xC4 => { mnemonic: "CPY", mode: :zero_page, desc: "Compare Y register with zero page value" },
    0xCC => { mnemonic: "CPY", mode: :absolute, desc: "Compare Y register with absolute value" },

    # DEC - Decrement
    0xC6 => { mnemonic: "DEC", mode: :zero_page, desc: "Decrement zero page value" },
    0xD6 => { mnemonic: "DEC", mode: :zero_page_x, desc: "Decrement zero page,X value" },
    0xCE => { mnemonic: "DEC", mode: :absolute, desc: "Decrement absolute value" },
    0xDE => { mnemonic: "DEC", mode: :absolute_x, desc: "Decrement absolute,X value" },
    0xCA => { mnemonic: "DEX", mode: :implicit, desc: "Decrement X register" },
    0x88 => { mnemonic: "DEY", mode: :implicit, desc: "Decrement Y register" },

    # EOR - Exclusive OR
    0x49 => { mnemonic: "EOR", mode: :immediate, desc: "XOR accumulator with immediate value" },
    0x45 => { mnemonic: "EOR", mode: :zero_page, desc: "XOR accumulator with zero page value" },
    0x55 => { mnemonic: "EOR", mode: :zero_page_x, desc: "XOR accumulator with zero page,X value" },
    0x4D => { mnemonic: "EOR", mode: :absolute, desc: "XOR accumulator with absolute value" },
    0x5D => { mnemonic: "EOR", mode: :absolute_x, desc: "XOR accumulator with absolute,X value" },
    0x59 => { mnemonic: "EOR", mode: :absolute_y, desc: "XOR accumulator with absolute,Y value" },
    0x41 => { mnemonic: "EOR", mode: :indexed_indirect, desc: "XOR accumulator with (zero page,X) value" },
    0x51 => { mnemonic: "EOR", mode: :indirect_indexed, desc: "XOR accumulator with (zero page),Y value" },

    # INC - Increment
    0xE6 => { mnemonic: "INC", mode: :zero_page, desc: "Increment zero page value" },
    0xF6 => { mnemonic: "INC", mode: :zero_page_x, desc: "Increment zero page,X value" },
    0xEE => { mnemonic: "INC", mode: :absolute, desc: "Increment absolute value" },
    0xFE => { mnemonic: "INC", mode: :absolute_x, desc: "Increment absolute,X value" },
    0xE8 => { mnemonic: "INX", mode: :implicit, desc: "Increment X register" },
    0xC8 => { mnemonic: "INY", mode: :implicit, desc: "Increment Y register" },

    # JMP - Jump
    0x4C => { mnemonic: "JMP", mode: :absolute, desc: "Jump to absolute address" },
    0x6C => { mnemonic: "JMP", mode: :indirect, desc: "Jump to indirect address" },

    # JSR - Jump to Subroutine
    0x20 => { mnemonic: "JSR", mode: :absolute, desc: "Jump to subroutine" },

    # LDA - Load Accumulator
    0xA9 => { mnemonic: "LDA", mode: :immediate, desc: "Load accumulator with immediate value" },
    0xA5 => { mnemonic: "LDA", mode: :zero_page, desc: "Load accumulator with zero page value" },
    0xB5 => { mnemonic: "LDA", mode: :zero_page_x, desc: "Load accumulator with zero page,X value" },
    0xAD => { mnemonic: "LDA", mode: :absolute, desc: "Load accumulator with absolute value" },
    0xBD => { mnemonic: "LDA", mode: :absolute_x, desc: "Load accumulator with absolute,X value" },
    0xB9 => { mnemonic: "LDA", mode: :absolute_y, desc: "Load accumulator with absolute,Y value" },
    0xA1 => { mnemonic: "LDA", mode: :indexed_indirect, desc: "Load accumulator with (zero page,X) value" },
    0xB1 => { mnemonic: "LDA", mode: :indirect_indexed, desc: "Load accumulator with (zero page),Y value" },

    # LDX - Load X Register
    0xA2 => { mnemonic: "LDX", mode: :immediate, desc: "Load X register with immediate value" },
    0xA6 => { mnemonic: "LDX", mode: :zero_page, desc: "Load X register with zero page value" },
    0xB6 => { mnemonic: "LDX", mode: :zero_page_y, desc: "Load X register with zero page,Y value" },
    0xAE => { mnemonic: "LDX", mode: :absolute, desc: "Load X register with absolute value" },
    0xBE => { mnemonic: "LDX", mode: :absolute_y, desc: "Load X register with absolute,Y value" },

    # LDY - Load Y Register
    0xA0 => { mnemonic: "LDY", mode: :immediate, desc: "Load Y register with immediate value" },
    0xA4 => { mnemonic: "LDY", mode: :zero_page, desc: "Load Y register with zero page value" },
    0xB4 => { mnemonic: "LDY", mode: :zero_page_x, desc: "Load Y register with zero page,X value" },
    0xAC => { mnemonic: "LDY", mode: :absolute, desc: "Load Y register with absolute value" },
    0xBC => { mnemonic: "LDY", mode: :absolute_x, desc: "Load Y register with absolute,X value" },

    # LSR - Logical Shift Right
    0x4A => { mnemonic: "LSR", mode: :accumulator, desc: "Shift accumulator right one bit" },
    0x46 => { mnemonic: "LSR", mode: :zero_page, desc: "Shift zero page value right one bit" },
    0x56 => { mnemonic: "LSR", mode: :zero_page_x, desc: "Shift zero page,X value right one bit" },
    0x4E => { mnemonic: "LSR", mode: :absolute, desc: "Shift absolute value right one bit" },
    0x5E => { mnemonic: "LSR", mode: :absolute_x, desc: "Shift absolute,X value right one bit" },

    # NOP - No Operation
    0xEA => { mnemonic: "NOP", mode: :implicit, desc: "No operation" },

    # ORA - Logical OR
    0x09 => { mnemonic: "ORA", mode: :immediate, desc: "OR accumulator with immediate value" },
    0x05 => { mnemonic: "ORA", mode: :zero_page, desc: "OR accumulator with zero page value" },
    0x15 => { mnemonic: "ORA", mode: :zero_page_x, desc: "OR accumulator with zero page,X value" },
    0x0D => { mnemonic: "ORA", mode: :absolute, desc: "OR accumulator with absolute value" },
    0x1D => { mnemonic: "ORA", mode: :absolute_x, desc: "OR accumulator with absolute,X value" },
    0x19 => { mnemonic: "ORA", mode: :absolute_y, desc: "OR accumulator with absolute,Y value" },
    0x01 => { mnemonic: "ORA", mode: :indexed_indirect, desc: "OR accumulator with (zero page,X) value" },
    0x11 => { mnemonic: "ORA", mode: :indirect_indexed, desc: "OR accumulator with (zero page),Y value" },

    # Stack Instructions
    0x48 => { mnemonic: "PHA", mode: :implicit, desc: "Push accumulator onto stack" },
    0x08 => { mnemonic: "PHP", mode: :implicit, desc: "Push processor status onto stack" },
    0x68 => { mnemonic: "PLA", mode: :implicit, desc: "Pull accumulator from stack" },
    0x28 => { mnemonic: "PLP", mode: :implicit, desc: "Pull processor status from stack" },

    # ROL - Rotate Left
    0x2A => { mnemonic: "ROL", mode: :accumulator, desc: "Rotate accumulator left through carry" },
    0x26 => { mnemonic: "ROL", mode: :zero_page, desc: "Rotate zero page value left through carry" },
    0x36 => { mnemonic: "ROL", mode: :zero_page_x, desc: "Rotate zero page,X value left through carry" },
    0x2E => { mnemonic: "ROL", mode: :absolute, desc: "Rotate absolute value left through carry" },
    0x3E => { mnemonic: "ROL", mode: :absolute_x, desc: "Rotate absolute,X value left through carry" },

    # ROR - Rotate Right
    0x6A => { mnemonic: "ROR", mode: :accumulator, desc: "Rotate accumulator right through carry" },
    0x66 => { mnemonic: "ROR", mode: :zero_page, desc: "Rotate zero page value right through carry" },
    0x76 => { mnemonic: "ROR", mode: :zero_page_x, desc: "Rotate zero page,X value right through carry" },
    0x6E => { mnemonic: "ROR", mode: :absolute, desc: "Rotate absolute value right through carry" },
    0x7E => { mnemonic: "ROR", mode: :absolute_x, desc: "Rotate absolute,X value right through carry" },

    # RTI - Return from Interrupt
    0x40 => { mnemonic: "RTI", mode: :implicit, desc: "Return from interrupt" },

    # RTS - Return from Subroutine
    0x60 => { mnemonic: "RTS", mode: :implicit, desc: "Return from subroutine" },

    # SBC - Subtract with Carry
    0xE9 => { mnemonic: "SBC", mode: :immediate, desc: "Subtract immediate value from accumulator with borrow" },
    0xE5 => { mnemonic: "SBC", mode: :zero_page, desc: "Subtract zero page value from accumulator with borrow" },
    0xF5 => { mnemonic: "SBC", mode: :zero_page_x, desc: "Subtract zero page,X value from accumulator with borrow" },
    0xED => { mnemonic: "SBC", mode: :absolute, desc: "Subtract absolute value from accumulator with borrow" },
    0xFD => { mnemonic: "SBC", mode: :absolute_x, desc: "Subtract absolute,X value from accumulator with borrow" },
    0xF9 => { mnemonic: "SBC", mode: :absolute_y, desc: "Subtract absolute,Y value from accumulator with borrow" },
    0xE1 => { mnemonic: "SBC", mode: :indexed_indirect, desc: "Subtract (zero page,X) value from accumulator with borrow" },
    0xF1 => { mnemonic: "SBC", mode: :indirect_indexed, desc: "Subtract (zero page),Y value from accumulator with borrow" },

    # STA - Store Accumulator
    0x85 => { mnemonic: "STA", mode: :zero_page, desc: "Store accumulator to zero page" },
    0x95 => { mnemonic: "STA", mode: :zero_page_x, desc: "Store accumulator to zero page,X" },
    0x8D => { mnemonic: "STA", mode: :absolute, desc: "Store accumulator to absolute address" },
    0x9D => { mnemonic: "STA", mode: :absolute_x, desc: "Store accumulator to absolute,X address" },
    0x99 => { mnemonic: "STA", mode: :absolute_y, desc: "Store accumulator to absolute,Y address" },
    0x81 => { mnemonic: "STA", mode: :indexed_indirect, desc: "Store accumulator to (zero page,X) address" },
    0x91 => { mnemonic: "STA", mode: :indirect_indexed, desc: "Store accumulator to (zero page),Y address" },

    # STX - Store X Register
    0x86 => { mnemonic: "STX", mode: :zero_page, desc: "Store X register to zero page" },
    0x96 => { mnemonic: "STX", mode: :zero_page_y, desc: "Store X register to zero page,Y" },
    0x8E => { mnemonic: "STX", mode: :absolute, desc: "Store X register to absolute address" },

    # STY - Store Y Register
    0x84 => { mnemonic: "STY", mode: :zero_page, desc: "Store Y register to zero page" },
    0x94 => { mnemonic: "STY", mode: :zero_page_x, desc: "Store Y register to zero page,X" },
    0x8C => { mnemonic: "STY", mode: :absolute, desc: "Store Y register to absolute address" },

    # Transfer Instructions
    0xAA => { mnemonic: "TAX", mode: :implicit, desc: "Transfer accumulator to X register" },
    0xA8 => { mnemonic: "TAY", mode: :implicit, desc: "Transfer accumulator to Y register" },
    0xBA => { mnemonic: "TSX", mode: :implicit, desc: "Transfer stack pointer to X register" },
    0x8A => { mnemonic: "TXA", mode: :implicit, desc: "Transfer X register to accumulator" },
    0x9A => { mnemonic: "TXS", mode: :implicit, desc: "Transfer X register to stack pointer" },
    0x98 => { mnemonic: "TYA", mode: :implicit, desc: "Transfer Y register to accumulator" }
  }

  def initialize
  end

  def format_operand(mode, bytes, address)
    case mode
    when :implicit, :accumulator
      ""
    when :immediate
      "#$%02X" % bytes[1]
    when :zero_page
      "$%02X" % bytes[1]
    when :zero_page_x
      "$%02X,X" % bytes[1]
    when :zero_page_y
      "$%02X,Y" % bytes[1]
    when :absolute
      "$%04X" % (bytes[1] | (bytes[2] << 8))
    when :absolute_x
      "$%04X,X" % (bytes[1] | (bytes[2] << 8))
    when :absolute_y
      "$%04X,Y" % (bytes[1] | (bytes[2] << 8))
    when :relative
      target = address + bytes[1] + 2
      target += 256 if bytes[1] > 127  # Handle negative offset
      target -= 256 if target > 0xFFFF
      "$%04X" % target
    when :indirect
      "$(%04X)" % (bytes[1] | (bytes[2] << 8))
    when :indexed_indirect
      "($%02X,X)" % bytes[1]
    when :indirect_indexed
      "($%02X),Y" % bytes[1]
    else
      "???"
    end
  end

  def disassemble_instruction(data, address)
    return nil if data.empty?
    
    opcode = data[0]
    # Convert to integer if it's a string byte
    opcode = opcode.ord if opcode.is_a?(String)
    instruction = INSTRUCTIONS[opcode]
    
    unless instruction
      return {
        bytes: [opcode],
        hex: "%02X" % opcode,
        mnemonic: "???",
        operand: "",
        desc: "Illegal or undocumented opcode",
        size: 1
      }
    end

    size = ADDRESSING_MODES[instruction[:mode]]
    bytes = data[0, size] || [opcode]
    
    # Convert string bytes to integers
    bytes = bytes.bytes if bytes.is_a?(String)
    
    # Handle case where we don't have enough bytes
    while bytes.length < size
      bytes << 0
    end

    operand = format_operand(instruction[:mode], bytes, address)
    hex_string = bytes.map { |b| "%02X" % b }.join(" ")

    {
      bytes: bytes,
      hex: hex_string,
      mnemonic: instruction[:mnemonic],
      operand: operand,
      desc: instruction[:desc] || "Unknown instruction",
      size: size
    }
  end
end

class NESRomParser
  attr_reader :header, :prg_rom, :chr_rom, :trainer

  INES_MAGIC = "NES\x1A".freeze

  def initialize(filename)
    @filename = filename
    @data = nil
    @header = {}
    @prg_rom = nil
    @chr_rom = nil
    @trainer = nil
  end

  def parse
    File.open(@filename, 'rb') do |file|
      @data = file.read
    end

    return false unless parse_header
    parse_data
    true
  end

  private

  def parse_header
    return false if @data.length < 16
    return false unless @data[0, 4] == INES_MAGIC

    # Convert bytes to integers for calculations
    bytes = @data.bytes
    
    @header = {
      magic: @data[0, 4],
      prg_rom_size: bytes[4] * 16 * 1024,  # 16KB units
      chr_rom_size: bytes[5] * 8 * 1024,   # 8KB units
      flags6: bytes[6],
      flags7: bytes[7],
      prg_ram_size: bytes[8] * 8 * 1024,   # 8KB units, 0 means 8KB for compatibility
      flags9: bytes[9],
      flags10: bytes[10],
      padding: @data[11, 5]
    }

    # Parse flags
    @header[:mapper] = ((@header[:flags7] & 0xF0) | (@header[:flags6] >> 4))
    @header[:mirroring] = @header[:flags6] & 0x01 == 0 ? :horizontal : :vertical
    @header[:battery_backed] = (@header[:flags6] & 0x02) != 0
    @header[:trainer_present] = (@header[:flags6] & 0x04) != 0
    @header[:four_screen] = (@header[:flags6] & 0x08) != 0
    @header[:vs_unisystem] = (@header[:flags7] & 0x01) != 0
    @header[:playchoice10] = (@header[:flags7] & 0x02) != 0

    # Set default PRG RAM size if 0
    @header[:prg_ram_size] = 8 * 1024 if @header[:prg_ram_size] == 0

    true
  end

  def parse_data
    offset = 16  # Header size

    # Trainer (512 bytes if present)
    if @header[:trainer_present]
      @trainer = @data[offset, 512]
      offset += 512
    end

    # PRG ROM
    if @header[:prg_rom_size] > 0
      @prg_rom = @data[offset, @header[:prg_rom_size]]
      offset += @header[:prg_rom_size]
    end

    # CHR ROM
    if @header[:chr_rom_size] > 0
      @chr_rom = @data[offset, @header[:chr_rom_size]]
    end
  end

  public

  def print_header
    puts "=" * 60
    puts "NES ROM Header Information"
    puts "=" * 60
    puts "File: #{@filename}"
    puts "Magic: #{@header[:magic].unpack('H*')[0].upcase}"
    puts "PRG ROM Size: #{@header[:prg_rom_size]} bytes (#{@header[:prg_rom_size] / 1024}KB)"
    puts "CHR ROM Size: #{@header[:chr_rom_size]} bytes (#{@header[:chr_rom_size] / 1024}KB)" 
    puts "PRG RAM Size: #{@header[:prg_ram_size]} bytes (#{@header[:prg_ram_size] / 1024}KB)"
    puts "Mapper: #{@header[:mapper]}"
    puts "Mirroring: #{@header[:mirroring].to_s.capitalize}"
    puts "Battery Backed: #{@header[:battery_backed] ? 'Yes' : 'No'}"
    puts "Trainer Present: #{@header[:trainer_present] ? 'Yes' : 'No'}"
    puts "Four Screen: #{@header[:four_screen] ? 'Yes' : 'No'}"
    puts "VS UniSystem: #{@header[:vs_unisystem] ? 'Yes' : 'No'}"
    puts "PlayChoice-10: #{@header[:playchoice10] ? 'Yes' : 'No'}"
    puts
  end

  def disassemble_prg_rom(start_offset = 0, length = nil, base_address = 0x8000)
    return unless @prg_rom

    disassembler = NES6502Disassembler.new
    
    # Default to entire PRG ROM if no length specified
    length ||= @prg_rom.length - start_offset
    length = [@prg_rom.length - start_offset, length].min

    puts "=" * 100
    puts "PRG ROM Disassembly (Offset: $%04X, Length: %d bytes, Base: $%04X)" % [start_offset, length, base_address]
    puts "=" * 100
    puts "Address  | Hex Code     | Assembly             ; Description"
    puts "-" * 100

    offset = start_offset
    address = base_address

    while offset < start_offset + length && offset < @prg_rom.length
      data = @prg_rom[offset..-1]
      break if data.empty?

      instruction = disassembler.disassemble_instruction(data, address)
      
      # Format the output with description
      hex_padding = " " * (12 - instruction[:hex].length)
      operand_part = instruction[:operand].empty? ? "" : " #{instruction[:operand]}"
      assembly_part = "#{instruction[:mnemonic]}#{operand_part}"
      
      puts "$%04X    | %-12s | %-20s ; %s" % [
        address,
        instruction[:hex],
        assembly_part,
        instruction[:desc]
      ]

      offset += instruction[:size]
      address += instruction[:size]
    end
    puts
  end

  def hex_dump(data, title, start_address = 0, max_bytes = 256)
    return unless data && !data.empty?

    puts "=" * 60
    puts "#{title} (#{data.length} bytes)"
    puts "=" * 60

    bytes_to_show = [data.length, max_bytes].min
    
    (0...bytes_to_show).step(16) do |i|
      # Address
      address = start_address + i
      print "$%04X: " % address

      # Hex bytes
      line_bytes = data[i, 16]
      hex_part = line_bytes.unpack('C*').map { |b| "%02X" % b }.join(' ')
      print "%-47s " % hex_part

      # ASCII part
      ascii_part = line_bytes.gsub(/[^\x20-\x7E]/, '.')
      puts "| #{ascii_part}"
    end

    if data.length > max_bytes
      puts "... (showing first #{max_bytes} of #{data.length} bytes)"
    end
    puts
  end
end

# Command line interface
def main
  options = {
    hex_dump: false,
    disasm: false,
    start_offset: 0,
    length: nil,
    base_address: 0x8000,
    max_hex: 256
  }

  OptionParser.new do |opts|
    opts.banner = "Usage: #{$0} [options] <nes_file>"
    opts.separator ""
    opts.separator "Options:"

    opts.on("-d", "--disasm", "Disassemble PRG ROM") do
      options[:disasm] = true
    end

    opts.on("-x", "--hex", "Show hex dump of ROM sections") do
      options[:hex_dump] = true
    end

    opts.on("-s", "--start OFFSET", Integer, "Start offset for disassembly (default: 0)") do |s|
      options[:start_offset] = s
    end

    opts.on("-l", "--length LENGTH", Integer, "Length of disassembly in bytes") do |l|
      options[:length] = l
    end

    opts.on("-b", "--base ADDRESS", Integer, "Base address for disassembly (default: 0x8000)") do |b|
      options[:base_address] = b
    end

    opts.on("-m", "--max-hex BYTES", Integer, "Maximum bytes to show in hex dump (default: 256)") do |m|
      options[:max_hex] = m
    end

    opts.on("-h", "--help", "Show this help message") do
      puts opts
      exit 0
    end
  end.parse!

  if ARGV.empty?
    puts "Error: Please specify a NES ROM file"
    puts "Use -h for help"
    exit 1
  end

  filename = ARGV[0]
  unless File.exist?(filename)
    puts "Error: File '#{filename}' not found"
    exit 1
  end

  begin
    parser = NESRomParser.new(filename)
    
    unless parser.parse
      puts "Error: Failed to parse NES ROM file"
      exit 1
    end

    # Always show header
    parser.print_header

    # Show hex dumps if requested
    if options[:hex_dump]
      if parser.trainer
        parser.hex_dump(parser.trainer, "Trainer", 0x7000, options[:max_hex])
      end
      
      if parser.prg_rom
        parser.hex_dump(parser.prg_rom, "PRG ROM", 0x8000, options[:max_hex])
      end
      
      if parser.chr_rom
        parser.hex_dump(parser.chr_rom, "CHR ROM", 0x0000, options[:max_hex])
      end
    end

    # Disassemble if requested
    if options[:disasm] && parser.prg_rom
      parser.disassemble_prg_rom(
        options[:start_offset],
        options[:length],
        options[:base_address]
      )
    end

    # If no specific options, show full ROM disassembly
    unless options[:hex_dump] || options[:disasm]
      puts "Full ROM Disassembly (use -d for custom range, -x for hex dump, -h for help)"
      puts
      
      # Disassemble entire PRG ROM
      if parser.prg_rom
        parser.disassemble_prg_rom(0, nil, 0x8000)
      end
    end

  rescue => e
    puts "Error: #{e.message}"
    exit 1
  end
end

# Run the program
if __FILE__ == $0
  main
end