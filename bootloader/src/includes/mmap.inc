bits 16

;
; Requests the memory map from bios
; Parameters:
;   di = destination for mmap
;
setup_mmap:
    pusha

    push di
    add di, 2                                   ; We will store the entry count in the first 2 bytes

    xor ax, ax                                  ; We will use ax as our counter
    xor ebx, ebx
    mov es, bx                                  ; Clear es as int 0x15 will use es:di for destination
    jmp .first                                  ; We dont want to check if ebx is zero on the first round
.loop:
    cmp ebx, 0
    je .end                                     ; If ebx is zero, it means we reached the end
.first:
    push ax                                     ; Push count
	mov [di + 20], dword 1                      ; Force a valid ACPI 3.X entry
    mov ax, 0xE820                              ; Memory map function
    mov ecx, 24                                 ; Asking for 24 bytes
    mov edx, 0x534D4150                         ; Magic number, "SMAP"
    int 0x15                                    ; Call bios with interrupt 15
    pop ax                                      ; Pop count
    jc .end                                     ; If carry is set, it means we reached the end and the current entry is invalid

    jcxz .loop                                  ; Ignore entry if cx is zero
    cmp cl, 20
    jbe .not_acpi3                              ; Skip forward if not acpi 3.0 entry
    test byte [di + 20], 1                      ; Test the ignore bit
    je .loop                                    ; If it is not set, skip
.not_acpi3:
    mov ecx, [di + 8]                           ; Get the lower 32 bits of length
	or ecx, [di + 12]                           ; Or it with the upper 32 bits to check for zero
    jz .loop                                    ; Skip if its zero
    add di, 24                                  ; Next entry
    inc ax                                      ; Increment count

    jmp .loop
.end:
    pop di
    mov [di], dword ax                          ; Store count
    popa
    ret