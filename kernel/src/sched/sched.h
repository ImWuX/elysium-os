#pragma once
#include <lib/list.h>
#include <sched/thread.h>

extern list_t g_sched_threads_all;
extern list_t g_sched_threads_queued;

void sched_thread_schedule(thread_t *thread);
thread_t *sched_thread_next();
void sched_thread_drop(thread_t *thread);