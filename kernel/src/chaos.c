#include "chaos.h"
#include <stdint.h>
#include <lib/assert.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <sched/sched.h>
#include <arch/sched.h>

static unsigned int seed;

unsigned int prand() {
    seed = (8253729 * seed + 2396403); 
    return seed  % 32767;
}

[[maybe_unused]] static void heap_chaos() {
    uint64_t allocs[10];
    for(int i = 0; i < 10; i++) allocs[i] = 0;

    while(true) {
        uint64_t val = prand();
        if(val % 400 == 0 || val % 200 == 0) continue;
        void *a = heap_alloc_align(val % 400, val % 200);
        ASSERT((uintptr_t) a % (val % 200) == 0);
        if(allocs[val % 10]) heap_free((void *) allocs[val % 10]);
        allocs[val % 10] = (uint64_t) a;
        asm volatile("pause");
    }
}

void chaos_tests_init() {
    uint32_t high, low;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    seed = high;

    // thread_t *heap_thread = arch_sched_thread_create_kernel(&heap_chaos);
    // sched_thread_schedule(heap_thread);
}