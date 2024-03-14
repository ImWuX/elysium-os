#include "syscall.h"
#include <errno.h>
#include <sys/utsname.h>
#include <lib/str.h>
#include <common/log.h>
#include <common/assert.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <arch/sched.h>

int syscall_buffer_out(void *dest, void *src, size_t count) {
    ASSERT(arch_sched_thread_current()->proc != NULL);
    return vmm_copy_to(arch_sched_thread_current()->proc->address_space, (uintptr_t) dest, src, count);
}

void *syscall_buffer_in(void *src, size_t count) {
    ASSERT(arch_sched_thread_current()->proc != NULL);
    void *buffer = heap_alloc(count);
    size_t read_count = vmm_copy_from(buffer, arch_sched_thread_current()->proc->address_space, (uintptr_t) src, count);
    if(read_count != count) {
        heap_free(buffer);
        return NULL;
    }
    return buffer;
}

int syscall_string_out(char *dest, char *src, size_t max) {
    size_t len = strlen(src) + 1;
    if(max < len) {
        log(LOG_LEVEL_WARN, "SYSCALL", "string_out: string length(%lu) exceeds maximum(%lu)", len, max);
        len = max;
    }
    return syscall_buffer_out(dest, src, len);
}

char *syscall_string_in(char *src, size_t length) {
    char *str = syscall_buffer_in(src, length + 1);
    if(str == NULL) return NULL;
    str[length] = 0;
    return str;
}

syscall_return_t syscall_debug(size_t length, char *str) {
    syscall_return_t ret = {};

    str = syscall_string_in(str, length);
    if(str == NULL) {
        heap_free(str);
        ret.err = EINVAL;
        return ret;
    }

    log(LOG_LEVEL_INFO, "MLIBC", "%s", str);

    heap_free(str);
    return ret;
}

syscall_return_t syscall_uname(struct utsname *buf) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "uname(buf: %#lx)", (uint64_t) buf);

    syscall_string_out(buf->sysname, "Elysium", sizeof(buf->sysname));
    syscall_string_out(buf->nodename, "elysium", sizeof(buf->nodename));
    syscall_string_out(buf->release, "pre-alpha", sizeof(buf->release));
    syscall_string_out(buf->version, "pre-alpha (" __DATE__ " " __TIME__ ")", sizeof(buf->version));

    return ret;
}