#pragma once
#include <stdint.h>
#include <lib/list.h>

#define TIME_NANOSECONDS_IN_SECOND 1'000'000'000

typedef struct {
    uint64_t seconds;
    uint32_t nanoseconds;
} time_t;

typedef struct timer {
    time_t deadline;
    void (* callback)(struct timer *timer);
    list_element_t list_elem;
} timer_t;

extern time_t g_monotonic_time;

/**
 * @brief Add a and b
 */
static inline time_t time_add(time_t a, time_t b) {
    a.seconds += b.seconds;
    a.nanoseconds += b.nanoseconds;
    if(a.nanoseconds >= TIME_NANOSECONDS_IN_SECOND) {
        a.nanoseconds -= TIME_NANOSECONDS_IN_SECOND;
        a.seconds++;
    }
    return a;
}

/**
 * @brief Subtract b from a
 */
static inline time_t time_subtract(time_t a, time_t b) {
    if(a.seconds < b.seconds) {
        a.seconds = a.nanoseconds = 0;
        return a;
    }
    a.seconds -= b.seconds;

    if(a.nanoseconds < b.nanoseconds) {
        if(a.seconds == 0) {
            a.seconds = a.nanoseconds = 0;
            return a;
        }
        a.nanoseconds += TIME_NANOSECONDS_IN_SECOND - b.nanoseconds;
        a.seconds--;
        return a;
    }
    a.nanoseconds -= b.nanoseconds;
    return a;
}

/**
 * @brief Advance time by some length
 */
void time_advance(time_t length);

/**
 * @brief Create timer
 */
timer_t *timer_create(time_t length, void (* callback)(timer_t *timer));