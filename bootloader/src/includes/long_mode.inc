bits 16

;
; Checks if long mode is available.
; If not it hangs
;
test_long_mode:
    pusha

    ; Check for CPUID
    pushfd
    pop eax
    mov ecx, eax

    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    xor eax, ecx
    jz .failed

    ; Check for CPUID extended info
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .failed

    ; Check for long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .failed

    popa
    ret

.failed:
    mov bx, TXT_LONG_MODE_ERROR
    call print
    cli
    hlt