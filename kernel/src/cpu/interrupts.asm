[extern exception_handler]
[extern irq_handler]

%macro PUSHALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsp
    push rbp
    push rsi
    push rdi
%endmacro

%macro POPALL 0
    pop rdi
    pop rsi
    pop rbp
    pop rsp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro STUB 2
    %1_stub:
        ; Save CPU state
        PUSHALL
        xor rax, rax ; TODO: Pointless code (we are only using ax anyway so why clear rax)
        mov ax, ds
        push rax

        ; Setting segment descriptors to kernel
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        cld

        ; Call Handler
        call %2

        ; Restore segment descriptors
        pop rax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        POPALL                              ; Restore CPU state
        add rsp, 16                         ; Pops error code and interrupt index
        sti
        iretq
%endmacro

%macro EXCEPTION_NOERRCODE 1
    global exception_%1
    exception_%1:
        cli
        push byte 0
        push byte %1
        jmp exception_stub
%endmacro

%macro EXCEPTION_ERRCODE 1
    global exception_%1
    exception_%1:
        cli
        push byte %1
        jmp exception_stub
%endmacro

%macro IRQ 1
    global irq_%1
    irq_%1:
        cli
        push 0
        push byte %1
        jmp irq_stub
%endmacro

STUB exception, exception_handler
STUB irq, irq_handler

; Exceptions
EXCEPTION_NOERRCODE 0
EXCEPTION_NOERRCODE 1
EXCEPTION_NOERRCODE 2
EXCEPTION_NOERRCODE 3
EXCEPTION_NOERRCODE 4
EXCEPTION_NOERRCODE 5
EXCEPTION_NOERRCODE 6
EXCEPTION_NOERRCODE 7
EXCEPTION_ERRCODE   8
EXCEPTION_NOERRCODE 9
EXCEPTION_ERRCODE   10
EXCEPTION_ERRCODE   11
EXCEPTION_ERRCODE   12
EXCEPTION_ERRCODE   13
EXCEPTION_ERRCODE   14
EXCEPTION_NOERRCODE 15
EXCEPTION_NOERRCODE 16
EXCEPTION_NOERRCODE 17
EXCEPTION_NOERRCODE 18
EXCEPTION_NOERRCODE 19
EXCEPTION_NOERRCODE 20
EXCEPTION_NOERRCODE 21
EXCEPTION_NOERRCODE 22
EXCEPTION_NOERRCODE 23
EXCEPTION_NOERRCODE 24
EXCEPTION_NOERRCODE 25
EXCEPTION_NOERRCODE 26
EXCEPTION_NOERRCODE 27
EXCEPTION_NOERRCODE 28
EXCEPTION_NOERRCODE 29
EXCEPTION_NOERRCODE 30
EXCEPTION_NOERRCODE 31

; IRQ
IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47