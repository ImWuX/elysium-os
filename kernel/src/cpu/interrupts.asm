extern exceptions_handler
extern irq_handler

%macro STUB 3
    %1_stub:
        push rax                            ; Save CPU state
        push rbx
        push rcx
        push rdx
        push rsp
        push rbp
        push rsi
        push rdi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15

        cld                                 ; Clear direction flag
        call %2                             ; Call appropriate interrupt handler

        pop r15                             ; Restore CPU state
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rdi
        pop rsi
        pop rbp
        pop rsp
        pop rdx
        pop rcx
        pop rbx
        pop rax

        add rsp, %3                         ; Discard interrupt number and other data off of the stack
        sti
        iretq
%endmacro

%macro EXCEPTION 2
    global exception_%1
    exception_%1:
        cli
        %if %1 == 0
            push qword 0                    ; Push 0 as error code for interrupt without error code
        %endif
        push qword %1                       ; Push interrupt number
        jmp exception_stub
%endmacro

%macro IRQ 1
    global irq_%1
    irq_%1:
        cli
        push qword %1                       ; Push interrupt number
        jmp irq_stub
%endmacro

STUB exception, exceptions_handler, 16
STUB irq, irq_handler, 8

EXCEPTION 0, 0
EXCEPTION 1, 0
EXCEPTION 2, 0
EXCEPTION 3, 0
EXCEPTION 4, 0
EXCEPTION 5, 0
EXCEPTION 6, 0
EXCEPTION 7, 0
EXCEPTION 8, 1
EXCEPTION 9, 0
EXCEPTION 10, 1
EXCEPTION 11, 1
EXCEPTION 12, 1
EXCEPTION 13, 1
EXCEPTION 14, 1
EXCEPTION 15, 0
EXCEPTION 16, 0
EXCEPTION 17, 0
EXCEPTION 18, 0
EXCEPTION 19, 0
EXCEPTION 20, 0
EXCEPTION 21, 0
EXCEPTION 22, 0
EXCEPTION 23, 0
EXCEPTION 24, 0
EXCEPTION 25, 0
EXCEPTION 26, 0
EXCEPTION 27, 0
EXCEPTION 28, 0
EXCEPTION 29, 0
EXCEPTION 30, 0
EXCEPTION 31, 0

%assign i 32
%rep 224
IRQ i
%assign i i+1
%endrep