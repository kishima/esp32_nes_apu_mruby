; NSF Continuous Tone Test
; 6502 Assembly source for continuous tone generation
; Target: $8000-$8020

.org $8000

; INIT routine - called once when NSF starts
init:
    LDA #$0F
    STA $4015       ; Enable all APU channels
    
    ; Pulse 1 setup (440Hz - A4)
    LDA #$BF        ; Duty=10 (50%), no halt, volume=15
    STA $4000
    LDA #$00        ; No sweep
    STA $4001
    LDA #$FD        ; Period low byte (440Hz)
    STA $4002
    LDA #$00        ; Period high byte
    STA $4003
    
    ; Pulse 2 setup (523Hz - C5)
    LDA #$BF        ; Duty=10 (50%), no halt, volume=15
    STA $4004
    LDA #$00        ; No sweep
    STA $4005
    LDA #$55        ; Period low byte (523Hz)
    STA $4006
    LDA #$00        ; Period high byte
    STA $4007
    
    RTS

; PLAY routine - called 60 times per second
.org $8020
play:
    LDA #$0F
    STA $4015       ; Keep channels enabled
    RTS