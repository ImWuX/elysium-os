#include <stdint.h>
#include <errno.h>
#include <common/log.h>
#include <sched/thread.h>
#include <syscall/syscall.h>
#include <arch/sched.h>
#include <arch/x86_64/sched.h>
#include <arch/x86_64/sys/msr.h>
#include <arch/x86_64/sys/gdt.h>

#define MSR_EFER_SCE 1

extern void x86_64_syscall_entry();

void x86_64_syscall_exit(int code) {
    log(LOG_LEVEL_DEBUG, "SYSCALL", "exit(code: %i, tid: %li)", code, arch_sched_thread_current()->id);
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    x86_64_sched_next();
    __builtin_unreachable();
}

syscall_return_t x86_64_syscall_debug(char c) {
    syscall_return_t ret = {};
    log_raw(c);
    return ret;
}

syscall_return_t x86_64_syscall_fs_set(void *ptr) {
    syscall_return_t ret = {};
    x86_64_msr_write(X86_64_MSR_FS_BASE, (uint64_t) ptr);
    log(LOG_LEVEL_DEBUG, "SYSCALL", "fs_set(ptr: %#lx)", (uint64_t) ptr);
    return ret;
}

void x86_64_syscall_init_cpu() {
    x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | MSR_EFER_SCE);
    x86_64_msr_write(X86_64_MSR_STAR, ((uint64_t) X86_64_GDT_CODE_RING0 << 32) | ((uint64_t) (X86_64_GDT_CODE_RING3 - 16) << 48));
    x86_64_msr_write(X86_64_MSR_LSTAR, (uint64_t) x86_64_syscall_entry);
    x86_64_msr_write(X86_64_MSR_SFMASK, x86_64_msr_read(X86_64_MSR_SFMASK) | (1 << 9));
}