#include <stddef.h>
#include <elysium/syscall.h>
#include <elib.h>

void *elib_acquire_framebuffer(elib_framebuffer_info_t *info) {
    uint64_t width, height, pitch;
    syscall_return_t ret = syscall3(SYSCALL_ELIB_FRAMEBUFFER, (syscall_int_t) &width, (syscall_int_t) &height, (syscall_int_t) &pitch);
    if(ret.err != 0) return NULL;
    info->width = width;
    info->height = height;
    info->pitch = pitch;
    return (void *) ret.value;
}

int elib_input() {
    return syscall0(SYSCALL_ELIB_INPUT).value;
}