#include "slock.h"
#include <stdint.h>
#include <lib/assert.h>

#define DEADLOCK_AT 10000000000

void slock_acquire(slock_t *lock) {
    uint64_t dead = 0;
    for(;;) {
        if(slock_try_acquire(lock)) break;
        ASSERT(dead++ != DEADLOCK_AT);
#ifdef __ARCH_AMD64
        asm volatile("pause");
#endif
    }
}