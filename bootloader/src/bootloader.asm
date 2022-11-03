bits 16
extern bootmain
extern ld_mmap
extern ld_temporary_paging
section .entry
jmp start                                       ; Jump from entry to .text section

section .text
start:
    xor ax, ax                                  ; Reset all segment registers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov bx, TXT_BOOT
    call print                                  ; Print boot message

    call enable_a20                             ; Enable A20
    call test_long_mode                         ; Test for long mode

    mov bx, TXT_LONG_MODE_AVAILABLE
    call print                                  ; Print long mode compatibility

    mov di, ld_mmap
    call setup_mmap

    mov edi, ld_temporary_paging
    call setup_paging                           ; Setup paging

    call setup_video_mode                       ; Change video mode

    cli                                         ; Clear interrupts as we are transitioning into long mode

    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr                                       ; Enable long mode

    lgdt [gdt.descriptor]                       ; Load GDT

    mov eax, cr0
    or eax, 1 << 31                             ; Set paging bit
    or eax, 1 << 0                              ; Set protected mode bit
    mov cr0, eax                                ; Enable paging and protected mode

    jmp CODE_SEGMENT:start_long                 ; Long jump to next segment

;
;   Includes
;
%include "includes/print.inc"
%include "includes/a20.inc"
%include "includes/long_mode.inc"
%include "includes/gdt.inc"
%include "includes/paging.inc"
%include "includes/vesa.inc"
%include "includes/mmap.inc"

;
;   Data
;
section .data
TXT_BOOT: db "Welcome to the NestOS bootloader", 0x0D, 0x0A, 0
TXT_LONG_MODE_AVAILABLE: db "CPU is x86_64 compatible", 0x0D, 0x0A, 0

TXT_A20_ERROR: db "Error: Failed to enable the A20 line", 0
TXT_LONG_MODE_ERROR: db "Error: CPU is not x86_64 compatible", 0
TXT_VESA_ERROR: db "Error: VESA not supported", 0
TXT_VIDEO_MODE_ERROR: db "Error: Specified video mode not found", 0

;
; 64 Bit Kernel Loader
;
bits 64
section .text
start_long:
    mov ax, DATA_SEGMENT                        ; Make sure our segment descriptors are set to data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp bootmain                                ; Jump to C code