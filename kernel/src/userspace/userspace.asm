global userspace_switch
userspace_switch:
	mov rax, rsp
	push qword 0x18 | 3
	push rax
	pushfq
	push qword 0x20 | 3
	push qword .entry
	iretq

.entry:
    jmp $