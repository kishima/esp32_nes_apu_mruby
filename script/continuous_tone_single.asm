; NSF Continuous Tone Test - Single Channel
; 6502 Assembly source for single channel tone generation
; Target: $8000-$8020

.org $8000

; INIT routine - called once when NSF starts
init:
    LDA #$01
    STA $4015       ; Enable only Pulse 1 channel
    
    ; Pulse 1 setup (440Hz - A4)
    LDA #$BF        ; Duty=10 (50%), no halt, volume=15
    STA $4000
    LDA #$00        ; No sweep
    STA $4001
    LDA #$FD        ; Period low byte (440Hz)
    STA $4002
    LDA #$00        ; Period high byte
    STA $4003
    
    RTS

; PLAY routine - called 60 times per second
.org $8020
play:
    LDA #$01
    STA $4015       ; Keep only Pulse 1 enabled
    RTS