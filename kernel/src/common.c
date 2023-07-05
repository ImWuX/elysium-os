#include "common.h"
#include <stdio.h>
#include <arch/types.h>
#include <arch/sched.h>
#include <memory/heap.h>
#include <sys/sched.h>
#include <istyx.h>

void common_init() {
    sched_thread_t *istyx_thread = heap_alloc(sizeof(sched_thread_t));
    arch_sched_init_kernel_thread(istyx_thread, istyx_thread_init);
    sched_add(istyx_thread);
}