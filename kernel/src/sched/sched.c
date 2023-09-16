#include "sched.h"
#include <arch/sched.h>

slock_t g_sched_threads_all_lock = SLOCK_INIT;
list_t g_sched_threads_all = LIST_INIT;
list_t g_sched_threads_queued = LIST_INIT_CIRCULAR(g_sched_threads_queued);
static slock_t g_lock = SLOCK_INIT;

void sched_thread_schedule(thread_t *thread) {
    slock_acquire(&g_lock);
    list_insert_before(&g_sched_threads_queued, &thread->list_sched);
    slock_release(&g_lock);
}

thread_t *sched_thread_next() {
    slock_acquire(&g_lock);
    if(list_is_empty(&g_sched_threads_queued)) {
        slock_release(&g_lock);
        return 0;
    }
    thread_t *thread = LIST_GET(g_sched_threads_queued.next, thread_t, list_sched);
    list_delete(&thread->list_sched);
    slock_release(&g_lock);
    return thread;
}

void sched_thread_drop(thread_t *thread) {
    if(thread == arch_sched_thread_current()->cpu->idle_thread) return;
    if(thread->state == THREAD_STATE_DESTROY) {
        arch_sched_thread_destroy(thread);
        return;
    }
    sched_thread_schedule(thread);
}