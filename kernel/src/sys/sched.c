#include "sched.h"

static sched_thread_t *g_threads = 0;

void sched_add(sched_thread_t *thread) {
    thread->next = g_threads;
    g_threads = thread;
}

sched_thread_t *sched_next_thread(sched_thread_t *current_thread) {
    sched_thread_t *thread = g_threads;
    while(thread && !__sync_bool_compare_and_swap(&thread->lock, 0, 1)) {
        thread = thread->next;
    }
    return thread;
}