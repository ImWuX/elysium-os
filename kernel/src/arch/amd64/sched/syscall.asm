SYSCALL_RSP_OFFSET equ 16
KERNEL_STACK_BASE_OFFSET equ 24

extern syscall_unimplemented
extern syscall_exit
extern syscall_debug
extern syscall_alloc_anon
extern syscall_fs_set
extern syscall_uname

section .data
syscall_table:
    dq syscall_exit         ; 0
    dq syscall_debug        ; 1
    dq syscall_alloc_anon   ; 2
    times 7 dq 0
    dq syscall_fs_set       ; 10
    dq syscall_uname        ; 11
.length: dq ($ - syscall_table) / 8

section .text
global syscall_entry
syscall_entry:
    swapgs
    mov qword [gs:SYSCALL_RSP_OFFSET], rsp
    mov rsp, qword [gs:KERNEL_STACK_BASE_OFFSET]

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

    mov r14, es
    push r14
    mov r14, ds
    push r14

    cmp rax, qword [syscall_table.length]
    jge .invalid_syscall

    mov rax, [syscall_table + rax * 8]
    cmp rax, 0
    je .invalid_syscall

    ; RDI, RSI, RDX contain the first 3 arguments, this also matches the first 3 arguments for the Sys V ABI.
    ; R10 contains the next argument, this does not match SysV. Lastly, R8 and R9 are passed which will match SysV again.
    mov rcx, r10
    call rax

    mov rbx, rdx ; Cannot use rdx for return value

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

    mov rsp, qword [gs:SYSCALL_RSP_OFFSET]
    swapgs
    o64 sysret