#pragma once
#include <stdint.h>

#define TIME_NANOSECONDS_IN_SECOND 1'000'000'000

typedef struct {
    uint64_t seconds;
    uint32_t nanoseconds;
} time_t;

static inline void time_add(time_t a, time_t b) {
    a.seconds += b.seconds;
    a.nanoseconds += b.nanoseconds;
    if(a.nanoseconds >= TIME_NANOSECONDS_IN_SECOND) {
        a.nanoseconds -= TIME_NANOSECONDS_IN_SECOND;
        a.seconds++;
    }
}

static inline void time_subtract(time_t a, time_t b) {
    if(a.seconds < b.seconds) {
        a.seconds = a.nanoseconds = 0;
        return;
    }
    a.seconds -= b.seconds;

    if(a.nanoseconds < b.nanoseconds) {
        if(a.seconds == 0) {
            a.seconds = a.nanoseconds = 0;
            return;
        }
        a.nanoseconds += TIME_NANOSECONDS_IN_SECOND - b.nanoseconds;
        a.seconds--;
        return;
    }
    a.nanoseconds -= b.nanoseconds;
}