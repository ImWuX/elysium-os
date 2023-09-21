#include "slock.h"
#include <stdint.h>
#include <lib/assert.h>
#include <arch/cpu.h>

#define DEADLOCK_AT 10000000000

void slock_acquire(volatile slock_t *lock) {
    uint64_t dead = 0;
    for(;;) {
        if(slock_try_acquire(lock)) return;
        ASSERT(dead++ != DEADLOCK_AT);

        while(__atomic_load_n(lock, __ATOMIC_RELAXED)) {
            arch_cpu_relax();
        }
    }
}