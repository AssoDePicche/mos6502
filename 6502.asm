        .org $0801      ; Program start address (C64 basic line)
        .byte $0c, $08, $0a, $00   ; Basic start line
        .byte $00, $00, $00, $00    ; Pointers
        .byte $9e, $32, $30, $36    ; "RUN" (ASCII)

        ; Main program
        .org $080d

start:
        ldx #$00           ; Initialize X register (index to characters)
loop:
        lda message, x     ; Load character from message
        beq done           ; If zero (end of string), we're done
        jsr $ffd2          ; C64 KERNAL routine to print character
        inx                ; Move to next character in string
        jmp loop           ; Repeat loop

done:
        ror                ; End of program

message:
        .ascii "Hello, World!"  ; The message
        .byte $00              ; Null terminator
