#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <elysium/syscall.h>

namespace mlibc {

    void sys_libc_log(const char *message) {
        syscall2(SYSCALL_DEBUG, (syscall_int_t) strlen(message), (syscall_int_t) message);
    }

    [[noreturn]] void sys_libc_panic() {
        syscall1(SYSCALL_EXIT, (syscall_int_t) 666);
        __builtin_unreachable();
    }

    [[noreturn]] void sys_exit(int status) {
        syscall1(SYSCALL_EXIT, (syscall_int_t) status);
        __builtin_unreachable();
    }

    int sys_tcb_set(void *pointer) {
        return syscall1(SYSCALL_FS_SET, (syscall_int_t) pointer).err;
    }

    int sys_futex_wait(int *pointer [[maybe_unused]], int expected [[maybe_unused]], const struct timespec *time [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_futex_wait called" << frg::endlog;
        return -1;
    }

    int sys_futex_wake(int *pointer [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_futex_wake called" << frg::endlog;
        return -1;
    }

    int sys_anon_allocate(size_t size, void **pointer) {
        syscall_return_t ret = syscall1(SYSCALL_ANON_ALLOCATE, size);
        if(ret.err != 0) return ret.err;
        *pointer = (void *) ret.value;
        return 0;
    }

    int sys_anon_free(void *pointer, size_t size) {
        return syscall2(SYSCALL_ANON_FREE, (syscall_int_t) pointer, size).err;
    }

    int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
        syscall_return_t ret = syscall5(SYSCALL_OPEN, (syscall_int_t) dirfd, (syscall_int_t) strlen(path), (syscall_int_t) path, (syscall_int_t) flags, (syscall_int_t) mode);
        if(ret.err) return ret.err;
        *fd = (int) ret.value;
        return 0;
    }

    int sys_open(const char *path, int flags, mode_t mode, int *fd) {
        return sys_openat(AT_FDCWD, path, flags, mode, fd);
    }

    int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
        syscall_return_t ret = syscall3(SYSCALL_READ, (syscall_int_t) fd, (syscall_int_t) buf, count);
        *bytes_read = (ssize_t) ret.value;
        return ret.err;
    }

    int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
        syscall_return_t ret = syscall3(SYSCALL_WRITE, (syscall_int_t) fd, (syscall_int_t) buf, count);
        *bytes_written = (ssize_t) ret.value;
        return ret.err;
    }

    int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
        syscall_return_t ret = syscall3(SYSCALL_SEEK, (syscall_int_t) fd, (syscall_int_t) offset, (syscall_int_t) whence);
        if(ret.err != 0) return ret.err;
        *new_offset = (off_t) ret.value;
        return 0;
    }

    int sys_close(int fd) {
        return syscall1(SYSCALL_CLOSE, (syscall_int_t) fd).err;
    }

    int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
        syscall_return_t ret;
        switch(fsfdt) {
            case fsfd_target::fd:
                ret = syscall5(SYSCALL_STAT, (syscall_int_t) fd, (syscall_int_t) 0, (syscall_int_t) "", (syscall_int_t) flags | AT_EMPTY_PATH, (syscall_int_t) statbuf);
                break;
            case fsfd_target::path:
                ret = syscall5(SYSCALL_STAT, (syscall_int_t) AT_FDCWD, (syscall_int_t) strlen(path), (syscall_int_t) path, (syscall_int_t) flags, (syscall_int_t) statbuf);
                break;
            case fsfd_target::fd_path:
                ret = syscall5(SYSCALL_STAT, (syscall_int_t) fd, (syscall_int_t) strlen(path), (syscall_int_t) path, (syscall_int_t) flags, (syscall_int_t) statbuf);
                break;
            default:
                __ensure(!"sys_stat: Invalid fsfdt");
                __builtin_unreachable();
        }
        return ret.err;
    }

    // mlibc assumes that anonymous memory returned by sys_vm_map() is zeroed by the kernel / whatever is behind the sysdeps
    int sys_vm_map(void *hint [[maybe_unused]], size_t size [[maybe_unused]], int prot [[maybe_unused]], int flags [[maybe_unused]], int fd [[maybe_unused]], off_t offset [[maybe_unused]], void **window [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_vm_map called" << frg::endlog;
        return -1;
    }

    int sys_vm_unmap(void *pointer [[maybe_unused]], size_t size [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_vm_unmap called" << frg::endlog;
        return -1;
    }

    int sys_vm_protect(void *pointer [[maybe_unused]], size_t size [[maybe_unused]], int prot [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_vm_protect called" << frg::endlog;
        return -1;
    }

    static int clock_get(int clock, syscall_clock_mode_t mode, time_t *secs, long *nanos) {
        syscall_clock_type_t type;
        switch(clock) {
            case CLOCK_REALTIME:
            case CLOCK_REALTIME_COARSE:
                type = SYSCALL_CLOCK_TYPE_REALTIME;
                break;
            case CLOCK_BOOTTIME:
            case CLOCK_MONOTONIC:
            case CLOCK_MONOTONIC_RAW:
            case CLOCK_MONOTONIC_COARSE:
                type = SYSCALL_CLOCK_TYPE_MONOTONIC;
                break;
            default: return EINVAL;
        }

        uint64_t seconds;
        uint32_t nanoseconds;
        syscall_return_t ret = syscall4(SYSCALL_CLOCK, (syscall_int_t) type, (syscall_int_t) mode, (syscall_int_t) &seconds, (syscall_int_t) &nanoseconds);
        if(ret.err != 0) return ret.err;
        *secs = (time_t) seconds;
        *nanos = (long) nanoseconds;
        return 0;
    }

    int sys_clock_getres(int clock, time_t *secs, long *nanos) {
        return clock_get(clock, SYSCALL_CLOCK_MODE_RES, secs, nanos);
    }

    int sys_clock_get(int clock, time_t *secs, long *nanos) {
        return clock_get(clock, SYSCALL_CLOCK_MODE_GET, secs, nanos);
    }

    int sys_isatty(int fd [[maybe_unused]]) {
        // TODO: Implement
        return ENOTTY;
    }

    int sys_uname(struct utsname *buf) {
        return syscall1(SYSCALL_UNAME, (syscall_int_t) buf).err;
    }

}