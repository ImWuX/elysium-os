bits 16

;
; Initializes paging.
; Parameters:
;   edi = address to load paging structures at
;
setup_paging:
    pusha

    mov cr3, edi                                ; Move it into cr3

    shr edi, 4
    mov es, di                                  ; Set es to edi / 0x10
    xor di, di                                  ; es:di = edi, we need to do this because rep stosd uses es:di
    xor eax, eax
    mov ecx, 4096                               ; Set how much memory we want to clear
    rep stosd                                   ; Clear memory
    xor bx, bx
    mov es, bx                                  ; Reset es to 0
    mov edi, cr3                                ; Move cr3 back

    mov eax, edi
    add eax, 0x1003
    mov dword [edi], eax                        ; Register PDPT into PMLT4

    add edi, 0x1000                             ; Move to PDPT
    mov eax, edi
    add eax, 0x1003
    mov dword [edi], eax                        ; Register PDT

    add edi, 0x1000                             ; Move to PDT
    mov ebx, 131                                ; Start paging at 0x0 (3 is the flag)
    mov ecx, 512                                ; Create 512 pages

.set_entry:
    mov dword [edi], ebx                        ; Register page
    add ebx, 0x200000                           ; Increase pointer to next page location
    add edi, 8                                  ; Move to next page
    loop .set_entry

    mov eax, cr4
    or eax, 1 << 5                              ; Enable PAE bit
    mov cr4, eax                                ; Move the modified register back

    popa
    ret