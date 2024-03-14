#include "time.h"
#include <common/spinlock.h>
#include <memory/heap.h>

time_t g_time_resolution = {};
time_t g_time_realtime = {};
time_t g_time_monotonic = {}; // TODO: this should be time since boot
static spinlock_t g_lock = SPINLOCK_INIT;
static list_t g_timers = LIST_INIT; // TODO: this needs to be a vector instead of a list (cuz linked list is def too slow here)

void time_advance(time_t length) {
    spinlock_acquire(&g_lock);

    g_time_monotonic = time_add(g_time_monotonic, length);
    g_time_realtime = time_add(g_time_realtime, length);

    LIST_FOREACH(&g_timers, elem) {
        timer_t *timer = LIST_CONTAINER_GET(elem, timer_t, list_elem);
        if(timer->deadline.seconds > g_time_monotonic.seconds) continue;
        if(timer->deadline.seconds == g_time_monotonic.seconds && timer->deadline.nanoseconds > g_time_monotonic.nanoseconds) continue;
        list_delete(&timer->list_elem);
        timer->callback(timer);
    }

    spinlock_release(&g_lock);
}

timer_t *timer_create(time_t length, void (* callback)(timer_t *timer)) {
    timer_t *timer = heap_alloc(sizeof(timer_t));
    timer->callback = callback;
    spinlock_acquire(&g_lock);
    timer->deadline = time_add(g_time_monotonic, length);
    list_append(&g_timers, &timer->list_elem);
    spinlock_release(&g_lock);
    return timer;
}