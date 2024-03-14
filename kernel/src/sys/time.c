#include "time.h"
#include <common/log.h>
#include <common/spinlock.h>

static time_t g_monotonic_time = {};
static spinlock_t g_lock = SPINLOCK_INIT;

void time_advance(time_t length) {
    spinlock_acquire(&g_lock);

    g_monotonic_time = time_add(g_monotonic_time, length);

    spinlock_release(&g_lock);
}