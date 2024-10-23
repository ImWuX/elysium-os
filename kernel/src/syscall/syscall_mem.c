#include <stdint.h>
#include <errno.h>
#include <common/log.h>
#include <lib/mem.h>
#include <syscall/syscall.h>
#include <memory/vmm.h>
#include <arch/types.h>
#include <arch/sched.h>

syscall_return_t syscall_mem_anon_allocate(uintptr_t size) {
    syscall_return_t ret = {};
    if(size == 0 || size % ARCH_PAGE_SIZE != 0) {
        ret.err = EINVAL;
        return ret;
    }
    void *p = vmm_map_anon(arch_sched_thread_current()->proc->address_space, NULL, size, VMM_PROT_READ | VMM_PROT_WRITE, VMM_FLAG_NONE, VMM_CACHE_STANDARD);
    memset(p, 0, size);
    ret.value = (uintptr_t) p;
    log(LOG_LEVEL_DEBUG, "SYSCALL", "anon_alloc(size: %#lx) -> %#lx", size, ret.value);
    return ret;
}

syscall_return_t syscall_mem_anon_free(void *pointer, size_t size) {
    syscall_return_t ret = {};
    if(
        size == 0 || size % ARCH_PAGE_SIZE != 0 ||
        pointer == NULL || ((uintptr_t) pointer) % ARCH_PAGE_SIZE != 0
    ) {
        ret.err = EINVAL;
        return ret;
    }
    // CRITICAL: ensure this is safe for userspace to just do (currently throws a kern panic...)
    vmm_unmap(arch_sched_thread_current()->proc->address_space, pointer, size);
    log(LOG_LEVEL_DEBUG, "SYSCALL", "anon_free(ptr: %#lx, size: %#lx)", (uint64_t) pointer, size);
    return ret;
}
