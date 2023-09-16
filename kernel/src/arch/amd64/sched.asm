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

    mov r11, 0x202
    sysretq