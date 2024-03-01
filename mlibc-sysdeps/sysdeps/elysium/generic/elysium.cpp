#include <mlibc/debug.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <elysium/syscall.h>
#include <errno.h>

namespace mlibc {

    void sys_libc_log(const char *message) {
        for(int i = 0; i < 9; i++) syscall1(SYSCALL_DEBUG, (syscall_int_t) "mlibc :: "[i]);
        for(int i = 0; message[i]; i++) syscall1(SYSCALL_DEBUG, (syscall_int_t) message[i]);
        syscall1(SYSCALL_DEBUG, (syscall_int_t) '\n');
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
        syscall_return_t ret = syscall1(SYSCALL_FS_SET, (syscall_int_t) pointer);
        if(ret.err != 0) return ret.err;
        return 0;
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
        syscall_return_t ret = syscall2(SYSCALL_ANON_FREE, (syscall_int_t) pointer, size);
        return ret.err;
    }

    int sys_openat(int dirfd [[maybe_unused]], const char *path [[maybe_unused]], int flags [[maybe_unused]], mode_t mode [[maybe_unused]], int *fd [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_openat called" << frg::endlog;
        return -1;
    }

    int sys_open(const char *path [[maybe_unused]], int flags [[maybe_unused]], mode_t mode [[maybe_unused]], int *fd [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_open called" << frg::endlog;
        return -1;
    }

    int sys_read(int fd [[maybe_unused]], void *buf [[maybe_unused]], size_t count [[maybe_unused]], ssize_t *bytes_read [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_read called" << frg::endlog;
        return -1;
    }

    int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
        syscall_return_t ret = syscall3(SYSCALL_WRITE, fd, (syscall_int_t) buf, count);
        *bytes_written = (ssize_t) ret.value;
        if(ret.err != 0) return ret.err;
        return 0;
    }

    int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
        syscall_return_t ret = syscall3(SYSCALL_SEEK, fd, (syscall_int_t) offset, (syscall_int_t) whence);
        if(ret.err != 0) return ret.err;
        *new_offset = (off_t) ret.value;
        return 0;
    }

    int sys_close(int fd [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_close called" << frg::endlog;
        return -1;
    }

    int sys_stat(fsfd_target fsfdt [[maybe_unused]], int fd [[maybe_unused]], const char *path [[maybe_unused]], int flags [[maybe_unused]], struct stat *statbuf [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_stat called" << frg::endlog;
        return -1;
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

    int sys_clock_get(int clock [[maybe_unused]], time_t *secs [[maybe_unused]], long *nanos [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_clock_get called" << frg::endlog;
        return -1;
    }

    int sys_isatty(int fd [[maybe_unused]]) {
        // TODO: Implement
        return ENOTTY;
    }

    int sys_uname(struct utsname *buf) {
        return syscall1(SYSCALL_UNAME, (syscall_int_t) buf).err;
    }

}