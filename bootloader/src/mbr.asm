org 0x7C5A
bits 16

BPB_OFFSET equ 0x7C00
LOAD_BUFFER equ 0x7E00                          ; 0x7C00 + 512      one sector wide buffer to temporarily load data at
FILE_DESTINATION equ 0x8000                     ; 0x7C00 + 1024     address to load both directory clusters and the destination file at

; FAT32 Bios Boot Parameters
BYTES_PER_SECTOR equ BPB_OFFSET + 11
SECTORS_PER_CLUSTER equ BPB_OFFSET + 13
FAT_COUNT equ BPB_OFFSET + 16
SECTORS_PER_FAT equ BPB_OFFSET + 36
RESERVED_SECTORS equ BPB_OFFSET + 14
ROOT_CLUSTER_NUM equ BPB_OFFSET + 44
DRIVE_NUMBER equ BPB_OFFSET + 64

;
; Trampoline To Boot
;
    cli                                         ; Disable interrupts for long jump to null segment
    jmp 0:boot                                  ; Jumping to segment 0 (BIOS can set segment to 0x7C00)

;
; Extra FAT32/DISK Data
;
first_cluster_offset: dd 0                      ; BPB + 96
sectors_per_track: dw 0                         ; BPB + 100
head_count: dw 0                                ; BPB + 102

;
; Boot
;
boot:
    xor ax, ax                                  ; Reset important segment registers
    mov es, ax
    mov ds, ax
    mov ss, ax
    cld                                         ; Set the direction flag to 0
    sti                                         ; Re-enable interrupts

    mov bp, BPB_OFFSET                          ; Set initial stack below the bootloader
    mov sp, bp

    mov [DRIVE_NUMBER], dl                      ; Store boot drive number (the field is unused and unreliable anyway)

    xor ax, ax                                  ; Use ah 0 interrupt
    int 0x13                                    ; Set disk system

    xor di, di
    mov ah, 0x08                                ; Use ah 8 interrupt
    int 0x13                                    ; Get disk data from bios
    jc error.disk                               ; Throw error if bios function failed
    and cl, 0x3F                                ; We don't need cylinder stuff so remove high two bits
    xor ch, ch
    mov [sectors_per_track], cx                 ; Save sectors per track
    inc dh                                      ; Starts at 0 in bios but we need to start from 1
    mov [head_count], dh                        ; Save head count

    mov ax, word [SECTORS_PER_FAT + 2]
    mov bx, word [SECTORS_PER_FAT]              ; ax:bx = sectors per fat
    movzx cx, byte [FAT_COUNT]                  ; cx = fat count
    mul cx                                      ; ax = total sector offset (high)
    xchg cx, ax                                 ; cx = total sector offset (high), ax = fat count
    mul bx                                      ; ax = total sector offset (low)
    add ax, word [RESERVED_SECTORS]
    adc dx, cx                                  ; dx:ax = total sector offset

    mov [first_cluster_offset], ax
    mov [first_cluster_offset + 2], dx          ; Save the offset of the first cluster

find_file:
    mov ax, word [ROOT_CLUSTER_NUM]
    mov dx, word [ROOT_CLUSTER_NUM + 2]
.search_cluster:
    push ax                                     ; Push cluster number onto stack
    push dx

    call cluster_to_lba                         ; dx:ax = LBA of the directory
    xor bx, bx                                  ; Use bx as a counter
    mov es, bx
.search_sector:
    push ax                                     ; Push the LBA onto the stack
    push dx
    push bx                                     ; Push sector count onto stack

    mov bx, LOAD_BUFFER
    mov cl, 1
    call read_disk                              ; Read one sector into buffer

    mov di, LOAD_BUFFER
    xor bx, bx
.search_entry:
    mov si, TXT_FILE_NAME                       ; What to compare
    mov cx, 11                                  ; How many character to compare
    push di
    repe cmpsb                                  ; Check if entry is equal to filename
    pop di
    je load_file

    add di, 32                                  ; Increment di by one entry (32 bytes)
    inc bx
    cmp bx, 16                                  ; Check if this sector is read
    jl .search_entry

    pop bx                                      ; Pop sector count into bx
    pop dx
    pop ax                                      ; Pop LBA into dx:ax

    inc bx
    add ax, 1                                   ; Increment LBA by one sector
    adc dx, 0
    cmp bl, byte [SECTORS_PER_CLUSTER]          ; Check if this is the last sector
    jb .search_sector

    pop dx
    pop ax                                      ; Pop current cluster number back into dx:ax
    call next_cluster                           ; Read next cluster number into dx:ax
    cmp dx, 0x0fff                              ; Check if last
    jne .search_cluster
    cmp ax, 0x0fff8                             ; Check if last
    jb .search_cluster
    jmp error.file                              ; Throw error if file was not found

load_file:
    add sp, 10
    mov dx, word [di + 20]
    mov ax, word [di + 26]
    mov [loadfile_offset], word FILE_DESTINATION
.loop:
    push ax
    push dx

    call cluster_to_lba
    mov cl, byte [SECTORS_PER_CLUSTER]
    mov bx, word [loadfile_offset + 2]
    mov es, bx
    mov bx, word [loadfile_offset]
    call read_disk                              ; Read current cluster of file

    xor ax, ax                                  ; Reset ax cuz its used for mult
    xor dx, dx
    mov al, byte [SECTORS_PER_CLUSTER]
    mov bx, word 512
    mul bx                                      ; dx:ax = amount of bytes in a cluster
    add [loadfile_offset], word ax              ; Increment loadfile dest by a cluster
    adc [loadfile_offset + 2], word dx

    pop dx
    pop ax

    call next_cluster
    cmp dx, 0x0fff                              ; Check if last
    jne .loop
    cmp ax, 0x0fff8                             ; Check if last
    jb .loop

    jmp FILE_DESTINATION                        ; Jump to file

;
; Errors
;
error:
.disk:
    mov bl, '0'
    jmp .end
.file:
    mov bl, '1'
.end:
    mov ah, 0x0e                                ; Set mode to teletype
    mov al, 'E'
    int 0x10
    mov al, ':'
    int 0x10
    mov al, bl
    int 0x10
    cli
    hlt

;
; Read next cluster number from FAT
; Parameters:
;   dx:ax = current cluster number
; Returns:
;   dx:ax = next cluster number
; Destroys: bx, cl
;
next_cluster:
    push ax
    shrd ax, dx, 7                              ; We'll only need bottom 7 bits
    shr dx, 7
    call cluster_to_lba.end                     ; Add cluster offset to ax and dx
    sub ax, word [SECTORS_PER_FAT]
    sbb dx, word [SECTORS_PER_FAT + 2]

    xor bx, bx
    mov es, bx                                  ; Clear ES
    mov bx, LOAD_BUFFER
    mov cl, 1                                   ; We only need the sector containing the next number
    call read_disk
    pop bx
    and bx, 0x7f
    shl bx, 2
    mov ax, word [LOAD_BUFFER + bx]
    mov dx, word [LOAD_BUFFER + bx + 2]
    and dx, 0x0fff
    ret

;
; Cluster number to LBA
; Parameters:
;   dx:ax = cluster number
; Returns:
;   dx:ax = LBA
; Destroys: cl
;
cluster_to_lba:
    sub ax, 2
    sbb dx, 0                                   ; Subtract 2 from dx:ax
    mov cl, byte [SECTORS_PER_CLUSTER]          ; Loop this many times
    jmp .start
.loop:
    shl ax, 1                                   ; Shift high bits
    rcl dx, 1                                   ; Rotate low bits with flag from high (This process multiplies the whole number by 2)
.start:
    shr cl, 1                                   ; Divide by 2
    jnz .loop                                   ; Only return if zero
.end:
    add ax, word [first_cluster_offset]
    adc dx, word [first_cluster_offset + 2]     ; Add offset
    ret

;
; Read sectors from disk
; Parameters:
;   dx:ax = LBA
;   cl = sectors to read
;   es:bx = destination address
; Destroys: bx, cx
;
read_disk:
    push ax
    push dx
    push bx
    push cx

    xchg ax, bx
    push dx
    mov ax, word [head_count]
    mul word [sectors_per_track]
    xchg ax, bx
    pop dx
    div bx
    xchg cx, ax
    xchg ax, dx
    div byte [sectors_per_track]
    mov dh, al
    mov dl, byte [DRIVE_NUMBER]

    xchg ch, cl
    shl cl, 6
    or cl, ah                                   ; Bunch of bit fuckery to get sector and cylinder aligned
    inc cx                                      ; Sectors start at 1

    pop ax                                      ; al = sectors to read
    pop bx                                      ; bx = destination
    mov ah, 0x02                                ; Set interrupt to read from disk
    int 0x13

    pop dx
    pop ax
    ret

;
; Data
;
TXT_FILE_NAME: db "BOOTLOADSYS"
loadfile_offset: dd 0

; Magic Number
times 420-($-$$) db 0
db 0x55, 0xaa