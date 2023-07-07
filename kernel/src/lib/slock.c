#include "slock.h"
#include <stdint.h>
#include <panic.h>

#define DEADLOCK_AT 10000000000

void slock_acquire(slock_t *lock) {
    uint64_t dead = 0;
    for(;;) {
        if(slock_try_acquire(lock)) break;
        if(dead++ == DEADLOCK_AT) panic("SLOCK", "Likely deadlock");
#ifdef __ARCH_AMD64
        asm volatile("pause");
#endif
    }
}