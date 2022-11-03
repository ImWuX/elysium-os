bits 16
extern ld_vbe_info
extern ld_vbe_mode_info

;
; Finds and sets the appropriate video mode
;
setup_video_mode:
    pusha
    call .find_video_mode

    mov bx, [eax]
    and bx, 0b11111111111100
    or bx,  0b00000000000010
    mov ax, 0x4F02
    int 0x10

    popa
    ret

.find_video_mode:
    mov [ld_vbe_info], byte 'V'
    mov [ld_vbe_info + 1], byte 'B'
    mov [ld_vbe_info + 2], byte 'E'
    mov [ld_vbe_info + 3], byte '2'

    mov ax, 0x4F00                              ; Function code for querying VESA bios info
    xor bx, bx
    mov es, bx                                  ; Set ES to zero
    mov di, ld_vbe_info                         ; Set location for info struct
    int 0x10                                    ; Call BIOS for VESA info

    cmp ax, 0x4F
    jne .error_unsupported

    push word [ld_vbe_info + 16]
    push word [ld_vbe_info + 14]
    pop eax                                     ; dword containing address of first video mode

.loop:
    push eax
    mov cx, [eax]
    mov ax, 0x4F01
    mov di, ld_vbe_mode_info
    int 0x10                                    ; Get info on video mode

    ; TODO: Fix this shit (Need a better way of doing this)

    mov ax, [ld_vbe_mode_info + 18]
    cmp ax, 1920                                ; Check width
    jne .next

    mov ax, [ld_vbe_mode_info + 20]
    cmp ax, 1080                                ; Check height
    jne .next

    mov al, [ld_vbe_mode_info + 25]
    cmp al, 0x20                                ; Check for RGBA
    jne .next

    mov al, [ld_vbe_mode_info + 27]
    cmp al, 0x6                                 ; Check memory model
    jne .next

    pop eax
    ret

.next:
    pop eax
    add eax, 2                                  ; Iterate to next mode
    cmp word [eax], 0xFFFF                      ; Termination code
    jne .loop

    mov bx, TXT_VIDEO_MODE_ERROR
    call print
    cli
    hlt

.error_unsupported:
    mov bx, TXT_VESA_ERROR
    call print
    cli
    hlt