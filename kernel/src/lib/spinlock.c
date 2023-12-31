#include "slock.h"
#include <stdint.h>
#include <lib/assert.h>
#include <arch/cpu.h>

#define DEADLOCK_AT 100000000

void spinlock_acquire(volatile slock_t *lock) {
    uint64_t dead = 0;
    for(;;) {
        if(spinlock_try_acquire(lock)) return;

        while(__atomic_load_n(lock, __ATOMIC_RELAXED)) {
            arch_cpu_relax();
            ASSERT(dead++ != DEADLOCK_AT);
        }
    }
}