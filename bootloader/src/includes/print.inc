bits 16

;
; Prints a string.
; Parameters:
;   bx: memory location of string
;
print:
    pusha
    mov ah, 0x0E                                ; Set mode to teletype
.loop:
    mov al, [bx]                                ; Get character
    cmp al, 0
    je .end                                     ; Check if it is null, if so, return
    int 0x10                                    ; Print character
    inc bx                                      ; Move to next character
    jmp .loop
.end:
    popa
    ret