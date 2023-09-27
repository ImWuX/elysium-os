THREAD_RSP_OFFSET equ 8

global sched_context_switch
sched_context_switch:
    push rbx
    push rbp
    push r15
    push r14
    push r13
    push r12

    mov qword [rdi + THREAD_RSP_OFFSET], rsp
    mov rsp, qword [rsi + THREAD_RSP_OFFSET]

    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    pop rbx

    mov rax, rdi
    ret

global sched_userspace_init
sched_userspace_init:
    pop rcx                                                 ; Pop return address into rcx (used by sysret)

    cli                                                     ; Clear interrupts as we are switching to a user stack
    swapgs

    pop rax                                                 ; Pop stack pointer
    mov rbp, rax
    mov rsp, rax

    push qword 0                                            ; TODO: Possibly dont push the invalid stack frame. Maybe this is up to the app?
    push qword 0

    xor rax, rax
    xor rbx, rbx
	xor rdx, rdx
	xor rsi, rsi
	xor rdi, rdi
	xor r8, r8
	xor r9, r9
	xor r10, r10
	xor r12, r12
	xor r13, r13
	xor r14, r14
	xor r15, r15

    mov r11, (1 << 9) | (1 << 1)                            ; Set the interrupt flag on sysret
    o64 sysret