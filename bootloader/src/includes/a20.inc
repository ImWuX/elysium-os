bits 16

KB_CMD equ 0x64
KB_DATA equ 0x60

;
; Attempts to enable the A20 line.
; If it fails, it hangs.
;
enable_a20:
    pusha

    call test_a20
    cmp ax, 1                                   ; Check if A20 is enabled
    je .success

    ; Try enabling A20 through BIOS
    mov ax, 0x2401
    int 0x15

    call test_a20
    cmp ax, 1                                   ; Check if A20 is enabled
    je .success

    ; Try enabling A20 through a keyboard controller
    cli
    call .wait_kb_write
    mov al, 0xAD                                ; 0xAD = Disable keyboard
    out KB_CMD, al

    call .wait_kb_write
    mov al, 0xD0                                ; 0xD0 = Read output port
    out KB_CMD, al

    call .wait_kb_data
    in al, KB_DATA                              ; Read byte from data port
    push ax

    call .wait_kb_write
    mov al, 0xD1                                ; 0xD1 = Write output port
    out KB_CMD, al

    call .wait_kb_write
    pop ax
    or al, 2                                    ; Set bit 2 of output port data (A20)
    out KB_DATA, al                             ; Write byte

    call .wait_kb_write
    mov al, 0xAE                                ; 0xAE = Enable keyboard
    out KB_CMD, al
    sti

    call test_a20
    cmp ax, 1                                   ; Check if A20 is enabled
    je .success

    ; Try enabling A20 using FastA20 (chipset)
    ; This could technically do anything, so we are doing this last
    in al, 0x92
    or al, 2
    out 0x92, al

    call test_a20
    cmp ax, 1                                   ; Check if A20 is enabled
    je .success

    mov bx, TXT_A20_ERROR
    call print                                  ; Print error

    cli
    hlt

.success:
    popa
    ret                                         ; A20 line enabled

.wait_kb_write:
    in al, 0x64
    test al, 2
    jnz .wait_kb_write
    ret

.wait_kb_data:
    in al, 0x64
    test al, 1
    jz .wait_kb_data
    ret

;
; Tests if the A20 line is enabled.
; Returns:
;   ax: 1 if the a20 line is disabled, 0 if it is enabled
;
test_a20:
    pusha

    mov ax, [0x7dfe]                            ; Get value at location of magic number

    push bx
    mov bx, 0xffff                              ; Set segment offset
    mov es, bx
    pop bx

    mov bx, 0x7e0e                              ; Location of magic number
    mov dx, [es:bx]                             ; Get value at location with offset 0xffff

    cmp ax, dx                                  ; Compare values, if they match we good!
    je .again                                   ; Test again

    popa
    mov ax, 1
    ret

.again:
    mov ax, [0x7dff]                            ; Get value at location of magic number

    push bx
    mov bx, 0xffff                              ; Set segment offset
    mov es, bx
    pop bx

    mov bx, 0x7e0f                              ; Location of magic number
    mov dx, [es:bx]                             ; Get value at location with offset 0xffff

    cmp ax, dx                                  ; Compare values, if they match we good!
    je .finish                                  ; Return

    popa
    mov ax, 1
    ret

.finish:
    popa
    xor ax, ax
    ret