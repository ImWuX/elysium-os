#include "syscall.h"
#include <lib/kprint.h>
#include <sched/sched.h>
#include <arch/sched.h>
#include <arch/amd64/sched/sched.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/gdt.h>

#define MAX_HANDLERS 2
#define MSR_EFER_SCE 1

extern void syscall_entry();

int64_t syscall_exit() {
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    sched_next();
    __builtin_unreachable();
}

int64_t syscall_write(uint64_t ch) {
    putchar((int) ch);
    return 0;
}

void syscall_init() {
    msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_SCE);
    msr_write(MSR_STAR, ((uint64_t) GDT_CODE_RING0 << 32) | ((uint64_t) (GDT_CODE_RING3 - 16) << 48));
    msr_write(MSR_LSTAR, (uint64_t) &syscall_entry);
    msr_write(MSR_SFMASK, msr_read(MSR_SFMASK) | (1 << 9));
}