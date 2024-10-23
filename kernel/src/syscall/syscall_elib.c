#include <lib/mem.h>
#include <lib/math.h>
#include <common/log.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <syscall/syscall.h>
#include <graphics/framebuffer.h>
#include <term.h>
#include <arch/types.h>
#include <arch/sched.h>

syscall_return_t syscall_elib_framebuffer(uint64_t *width, uint64_t *height, uint64_t *pitch) {
    syscall_return_t ret = {};

    log(LOG_LEVEL_DEBUG, "SYSCALL", "elib_framebuffer()");

    term_close();

    void *dest = vmm_map_direct(
        arch_sched_thread_current()->proc->address_space,
        NULL,
        MATH_CEIL(g_framebuffer.size, ARCH_PAGE_SIZE),
        VMM_PROT_READ | VMM_PROT_WRITE,
        VMM_FLAG_NONE,
        VMM_CACHE_WRITE_COMBINE,
        g_framebuffer.phys_address
    );

    if(dest == NULL) goto err;
    if(syscall_buffer_out(width, &g_framebuffer.width, sizeof(uint64_t)) != sizeof(uint64_t)) goto err;
    if(syscall_buffer_out(height, &g_framebuffer.height, sizeof(uint64_t)) != sizeof(uint64_t)) goto err;
    if(syscall_buffer_out(pitch, &g_framebuffer.pitch, sizeof(uint64_t)) != sizeof(uint64_t)) goto err;
    ret.value = (uintptr_t) dest;
    return ret;

    err:
    ret.err = 1;
    return ret;
}