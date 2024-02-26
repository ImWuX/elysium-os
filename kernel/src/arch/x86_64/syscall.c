#include <stdint.h>
#include <errno.h>
#include <sys/utsname.h>
#include <lib/str.h>
#include <common/kprint.h>
#include <sched/thread.h>
#include <arch/types.h>
#include <arch/sched.h>
#include <arch/x86_64/sched.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/gdt.h>

#define MSR_EFER_SCE 1

typedef struct {
    uint64_t value;
    uint64_t errno;
} syscall_return_t;

extern void x86_64_syscall_entry();

void x86_64_syscall_exit(int code) {
    kprintf("syscall :: exit(code: %i, tid: %li) -> exit\n", code, arch_sched_thread_current()->id);
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    x86_64_sched_next();
    __builtin_unreachable();
}

syscall_return_t x86_64_syscall_debug(char c) {
    syscall_return_t ret = {};
    kprintf("%c", c);
    return ret;
}

syscall_return_t x86_64_syscall_anon_allocate(uintptr_t size) {
    syscall_return_t ret = {};
    if(size == 0 || size % ARCH_PAGE_SIZE) {
        ret.errno = EINVAL;
        return ret;
    }
    void *p = vmm_map(arch_sched_thread_current()->proc->address_space, NULL, size, VMM_PROT_READ | VMM_PROT_WRITE, VMM_FLAG_NONE, &g_seg_anon, NULL);
    ret.value = (uintptr_t) p;
    kprintf("syscall :: alloc_anon(size: %#lx) -> %#lx\n", size, ret.value);
    return ret;
}

syscall_return_t x86_64_syscall_fs_set(void *ptr) {
    syscall_return_t ret = {};
    x86_64_msr_write(X86_64_MSR_FS_BASE, (uint64_t) ptr);
    kprintf("syscall :: fs_set(ptr: %#lx) -> void\n", (uint64_t) ptr);
    return ret;
}

syscall_return_t x86_64_syscall_uname(struct utsname *buf) {
    syscall_return_t ret = {};

    strncpy(buf->sysname, "Elysium", sizeof(buf->sysname));
    strncpy(buf->nodename, "elysium", sizeof(buf->nodename));
    strncpy(buf->release, "pre-alpha", sizeof(buf->release));
    strncpy(buf->version, "pre-alpha (" __DATE__ " " __TIME__ ")", sizeof(buf->version));

    return ret;
}

void x86_64_syscall_init() {
    x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | MSR_EFER_SCE);
    x86_64_msr_write(X86_64_MSR_STAR, ((uint64_t) X86_64_GDT_CODE_RING0 << 32) | ((uint64_t) (X86_64_GDT_CODE_RING3 - 16) << 48));
    x86_64_msr_write(X86_64_MSR_LSTAR, (uint64_t) x86_64_syscall_entry);
    x86_64_msr_write(X86_64_MSR_SFMASK, x86_64_msr_read(X86_64_MSR_SFMASK) | (1 << 9));
}