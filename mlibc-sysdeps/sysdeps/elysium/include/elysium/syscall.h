#ifndef _ELYSIUM__SYSCALL_H
#define _ELYSIUM__SYSCALL_H

#include <stdint.h>

#define SYSCALL_EXIT 0
#define SYSCALL_DEBUG 1
#define SYSCALL_ANON_ALLOCATE 2
#define SYSCALL_ANON_FREE 3
#define SYSCALL_FS_SET 4
#define SYSCALL_UNAME 5
#define SYSCALL_OPEN 6
#define SYSCALL_CLOSE 7
#define SYSCALL_WRITE 8
#define SYSCALL_READ 9
#define SYSCALL_SEEK 10
#define SYSCALL_STAT 11
#define SYSCALL_ELIB_FRAMEBUFFER 12
#define SYSCALL_CLOCK 13

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t syscall_int_t;

typedef struct {
    syscall_int_t value;
    syscall_int_t err;
} syscall_return_t;

typedef enum {
    SYSCALL_CLOCK_TYPE_REALTIME,
    SYSCALL_CLOCK_TYPE_MONOTONIC
} syscall_clock_type_t;

typedef enum {
    SYSCALL_CLOCK_MODE_RES,
    SYSCALL_CLOCK_MODE_GET,
    SYSCALL_CLOCK_MODE_SET
} syscall_clock_mode_t;

#ifdef __cplusplus
}
#endif

#ifndef __MLIBC_ABI_ONLY
#define DEFINE_SYSCALL(...)                     \
    syscall_return_t ret;                       \
    asm volatile("syscall"                      \
        : "=a" (ret.value), "=b" (ret.err)      \
        : __VA_ARGS__                           \
        : "rcx", "r11", "memory"                \
    );                                          \
    return ret;

#ifdef __cplusplus
extern "C" {
#endif

    static syscall_return_t syscall0(int sc) {
        DEFINE_SYSCALL("a" (sc));
    }

    static syscall_return_t syscall1(int sc, syscall_int_t arg1) {
        DEFINE_SYSCALL("a" (sc), "D" (arg1));
    }

    static syscall_return_t syscall2(int sc, syscall_int_t arg1, syscall_int_t arg2) {
        DEFINE_SYSCALL("a" (sc), "D" (arg1), "S" (arg2));
    }

    static syscall_return_t syscall3(int sc, syscall_int_t arg1, syscall_int_t arg2, syscall_int_t arg3) {
        DEFINE_SYSCALL("a" (sc), "D" (arg1), "S" (arg2), "d" (arg3));
    }

    static syscall_return_t syscall4(int sc, syscall_int_t arg1, syscall_int_t arg2, syscall_int_t arg3, syscall_int_t arg4) {
        register syscall_int_t arg4_reg asm("r10") = arg4;
        DEFINE_SYSCALL("a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg));
    }

    static syscall_return_t syscall5(int sc, syscall_int_t arg1, syscall_int_t arg2, syscall_int_t arg3, syscall_int_t arg4, syscall_int_t arg5) {
        register syscall_int_t arg4_reg asm("r10") = arg4;
        register syscall_int_t arg5_reg asm("r8") = arg5;
        DEFINE_SYSCALL("a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg), "r" (arg5_reg));
    }

    static syscall_return_t syscall6(int sc, syscall_int_t arg1, syscall_int_t arg2, syscall_int_t arg3, syscall_int_t arg4, syscall_int_t arg5, syscall_int_t arg6) {
        register syscall_int_t arg4_reg asm("r10") = arg4;
        register syscall_int_t arg5_reg asm("r8") = arg5;
        register syscall_int_t arg6_reg asm("r9") = arg6;
        DEFINE_SYSCALL("a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg), "r" (arg5_reg), "r" (arg6_reg));
    }

#ifdef __cplusplus
}
#endif
#endif

#endif