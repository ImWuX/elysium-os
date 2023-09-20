SYSCALL_RSP_OFFSET equ 16
KERNEL_STACK_BASE_OFFSET equ 24

extern syscall_exit
extern syscall_putchar

section .data
syscall_table:
.start:
    dq syscall_exit         ; 0
    dq syscall_putchar      ; 1
.end:
.length: dq (.end - .start) / 8

section .text
global syscall_entry
syscall_entry:
    swapgs
    mov qword [gs:SYSCALL_RSP_OFFSET], rsp
    mov rsp, qword [gs:KERNEL_STACK_BASE_OFFSET]

    push rbx
    push rcx
    push rdx
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

    mov rdi, es
    push rdi
    mov rdi, ds
    push rdi

    cmp rax, qword [syscall_table.length]
    jge .invalid_syscall

    mov rdi, rbx
    mov rsi, rcx
    mov rsi, rcx
    call [syscall_table + rax * 8]

    .invalid_syscall:

    pop rdi
    mov ds, rdi
    pop rdi
    mov es, rdi

    pop r15
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
    pop rdx
    pop rcx
    pop rbx

    mov rsp, qword [gs:SYSCALL_RSP_OFFSET]
    swapgs
    o64 sysret