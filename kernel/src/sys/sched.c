#include "sched.h"
#include <panic.h>

static list_t g_queue = LIST_INIT_CIRCULAR(g_queue);
static uint32_t g_ids = 0;
static slock_t g_sched_lock = SLOCK_INIT;

void sched_add(sched_thread_t *thread) {
    thread->id = g_ids++;
    if(g_ids == UINT32_MAX) panic("SCHED", "Exceeded thread id limit");
    sched_schedule_thread(thread);
}

sched_thread_t *sched_next_thread() {
    slock_acquire(&g_sched_lock);
    if(list_is_empty(&g_queue)) {
        slock_release(&g_sched_lock);
        return 0;
    }
    sched_thread_t *thread = list_get(g_queue.next, sched_thread_t, list);
    list_delete(&thread->list);
    slock_release(&g_sched_lock);
    thread->state = SCHED_THREAD_ACTIVE;
    return thread;
}

void sched_schedule_thread(sched_thread_t *thread) {
    thread->state = SCHED_THREAD_READY;
    slock_acquire(&g_sched_lock);
    list_insert_before(&g_queue, &thread->list);
    slock_release(&g_sched_lock);
}