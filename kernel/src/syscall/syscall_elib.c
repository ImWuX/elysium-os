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

    seg_fixed_data_t *fixed_data = heap_alloc(sizeof(seg_fixed_data_t));
    memset(fixed_data, 0, sizeof(seg_fixed_data_t));
    fixed_data->phys_base = g_framebuffer.phys_address;
    void *dest = vmm_map(arch_sched_thread_current()->proc->address_space, NULL, MATH_CEIL(g_framebuffer.size, ARCH_PAGE_SIZE), VMM_PROT_READ | VMM_PROT_WRITE, VMM_FLAG_NONE, &g_seg_fixed, fixed_data);

    ret.err = 1;
    if(dest == NULL) return ret;
    if(syscall_buffer_out(width, &g_framebuffer.width, sizeof(uint64_t)) != sizeof(uint64_t)) return ret;
    if(syscall_buffer_out(height, &g_framebuffer.height, sizeof(uint64_t)) != sizeof(uint64_t)) return ret;
    if(syscall_buffer_out(pitch, &g_framebuffer.pitch, sizeof(uint64_t)) != sizeof(uint64_t)) return ret;
    ret.err = 0;
    ret.value = (uintptr_t) dest;
    return ret;
}