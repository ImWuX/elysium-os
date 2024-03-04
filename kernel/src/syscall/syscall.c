#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <lib/str.h>
#include <common/log.h>
#include <memory/heap.h>
#include <syscall/syscall.h>

syscall_return_t syscall_debug(size_t length, char *str) {
    syscall_return_t ret = {};

    // TODO: Should standardize copying to/from userspace
    char *safe_str = heap_alloc(length + 1);
    strncpy(safe_str, str, length);
    safe_str[length] = 0;

    log(LOG_LEVEL_INFO, "MLIBC", "%s", safe_str);
    return ret;
}

syscall_return_t syscall_uname(struct utsname *buf) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "uname(buf: %#lx)", (uint64_t) buf);

    // TODO: Should standardize copying to/from userspace
    strncpy(buf->sysname, "Elysium", sizeof(buf->sysname));
    strncpy(buf->nodename, "elysium", sizeof(buf->nodename));
    strncpy(buf->release, "pre-alpha", sizeof(buf->release));
    strncpy(buf->version, "pre-alpha (" __DATE__ " " __TIME__ ")", sizeof(buf->version));

    return ret;
}