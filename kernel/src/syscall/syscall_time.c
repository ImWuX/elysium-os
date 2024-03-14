#include <stdint.h>
#include <errno.h>
#include <common/log.h>
#include <syscall/syscall.h>
#include <sys/time.h>

syscall_return_t syscall_time_clock(int clock, int mode, uint64_t *seconds, uint32_t *nanoseconds) {
    syscall_return_t ret = {};

    log(LOG_LEVEL_DEBUG, "SYSCALL", "time_clock(clock: %i, mode: %i)", clock, mode);

    if(mode == SYSCALL_CLOCK_MODE_SET) {
        ret.err = ENOTSUP;
        return ret;
    }

    if(mode != SYSCALL_CLOCK_MODE_RES && mode != SYSCALL_CLOCK_MODE_GET) {
        ret.err = EINVAL;
        return ret;
    }

    time_t time = {};
    switch(clock) {
        case SYSCALL_CLOCK_TYPE_REALTIME:
            switch(mode) {
                case SYSCALL_CLOCK_MODE_RES: time = g_time_resolution; break;
                case SYSCALL_CLOCK_MODE_GET: time = g_time_realtime; break;
            }
            break;
        case SYSCALL_CLOCK_TYPE_MONOTONIC:
            switch(mode) {
                case SYSCALL_CLOCK_MODE_RES: time = g_time_resolution; break;
                case SYSCALL_CLOCK_MODE_GET: time = g_time_monotonic; break;
            }
            break;
        default:
            ret.err = EINVAL;
            return ret;
    }

    if(syscall_buffer_out(seconds, &time.seconds, sizeof(uint64_t)) != sizeof(uint64_t)) ret.err = EINVAL;
    if(syscall_buffer_out(nanoseconds, &time.nanoseconds, sizeof(uint32_t)) != sizeof(uint32_t)) ret.err = EINVAL;
    return ret;
}