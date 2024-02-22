#include "sched.h"
#include <lib/container.h>
#include <sched/thread.h>
#include <arch/x86_64/msr.h>

#define X86_64_THREAD(THREAD) (CONTAINER_OF((THREAD), x86_64_thread_t, common))

typedef struct x86_64_thread {
    struct x86_64_thread *this;
    thread_t common;
} x86_64_thread_t;

thread_t *arch_sched_thread_current() {
    x86_64_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return &thread->common;
}