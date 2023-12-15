#include "syscall.h"
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/slock.h>
#include <lib/round.h>
#include <lib/kprint.h>
#include <lib/c/errno.h>
#include <lib/c/string.h>
#include <memory/heap.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>
#include <sched/sched.h>
#include <arch/sched.h>
#include <arch/vmm.h>
#include <arch/amd64/sched/sched.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <graphics/draw.h>

#define MSR_EFER_SCE 1

typedef struct {
    uint64_t value;
    uint64_t errno;
} syscall_return_t;

extern void syscall_entry();

void syscall_exit(int code) {
    kprintf("syscall :: exit(code: %i, tid: %li) -> exit\n", code, arch_sched_thread_current()->id);
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    sched_next();
    __builtin_unreachable();
}

syscall_return_t syscall_debug(char c) {
    syscall_return_t ret = {};
    putchar(c);
    return ret;
}

syscall_return_t syscall_alloc_anon(size_t size) {
    syscall_return_t ret = {};
    if(size == 0 || size % ARCH_PAGE_SIZE) {
        ret.errno = EINVAL;
        return ret;
    }
    static uintptr_t address = 0x50000000;
    int r;
    while(true) {
        r = vmm_map_anon(arch_sched_thread_current()->proc->address_space, address, size, VMM_PROT_USER | VMM_PROT_WRITE, false);
        if(r == 0) break;
        address += ARCH_PAGE_SIZE;
    }
    ret.value = address;
    address += size;
    kprintf("syscall :: alloc_anon(size: %#lx) -> %#lx\n", size, ret.value);
    return ret;
}

syscall_return_t syscall_fs_set(void *ptr) {
    syscall_return_t ret = {};
    msr_write(MSR_FS_BASE, (uint64_t) ptr);
    kprintf("syscall :: fs_set(ptr: %#lx) -> void\n", (uint64_t) ptr);
    return ret;
}

// static slock_t g_kbq_lock = SLOCK_INIT;
// static list_t g_kbq = LIST_INIT_CIRCULAR(g_kbq);

// static void consume_kb_event(uint8_t ch) {
//     kb_event_t *kbev = heap_alloc(sizeof(kb_event_t));
//     kbev->ch = ch;
//     slock_acquire(&g_kbq_lock);
//     list_insert_before(&g_kbq, &kbev->list);
//     slock_release(&g_kbq_lock);
// }

void syscall_init() {
    msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_SCE);
    msr_write(MSR_STAR, ((uint64_t) GDT_CODE_RING0 << 32) | ((uint64_t) (GDT_CODE_RING3 - 16) << 48));
    msr_write(MSR_LSTAR, (uint64_t) &syscall_entry);
    msr_write(MSR_SFMASK, msr_read(MSR_SFMASK) | (1 << 9));

    // ps2kb_set_handler(&consume_kb_event);
}