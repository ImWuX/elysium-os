THREAD_KERNEL_RSP_OFFSET equ 8

global sched_context_switch
sched_context_switch:
    push rbx
    push rbp
    push r15
    push r14
    push r13
    push r12

    mov qword [rdi + THREAD_KERNEL_RSP_OFFSET], rsp
    mov rsp, qword [rsi + THREAD_KERNEL_RSP_OFFSET]

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
    swapgs
    pop rcx

    mov r11, 0x202
    sysretq