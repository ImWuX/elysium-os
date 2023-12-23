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
        syscall_return_t ret = syscall1(SYSCALL_ALLOC_ANON, (syscall_int_t) size);
        if(ret.err != 0) return ret.err;
        *pointer = (void *) ret.value;
        return 0;
    }

    int sys_anon_free(void *pointer [[maybe_unused]], size_t size [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_anon_free called" << frg::endlog;
        return -1;
    }

    int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
        // syscall_return_t ret = syscall4(SYSCALL_OPEN, (syscall_int_t) dirfd, (syscall_int_t) path, (syscall_int_t) flags, (syscall_int_t) mode);
        // if(ret.err != 0) return ret.err;
        // *fd = ret.value;
        mlibc::infoLogger() << "unimplemented sys_open called" << frg::endlog;
        return 0;
    }

    int sys_open(const char *path, int flags, mode_t mode, int *fd) {
        return sys_openat(AT_FDCWD, path, flags, mode, fd);
    }

    int sys_read(int fd [[maybe_unused]], void *buf [[maybe_unused]], size_t count [[maybe_unused]], ssize_t *bytes_read [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_read called" << frg::endlog;
        return 0;
    }

    int sys_write(int fd [[maybe_unused]], const void *buf [[maybe_unused]], size_t count [[maybe_unused]], ssize_t *bytes_written [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_write called" << frg::endlog;
        return 0;
    }

    int sys_seek(int fd [[maybe_unused]], off_t offset [[maybe_unused]], int whence [[maybe_unused]], off_t *new_offset [[maybe_unused]]) {
        // TODO: Implement
        mlibc::infoLogger() << "unimplemented sys_seek called" << frg::endlog;
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
        return 0;
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