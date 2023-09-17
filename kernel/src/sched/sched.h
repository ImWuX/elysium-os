#pragma once
#include <lib/list.h>
#include <lib/slock.h>
#include <sched/thread.h>
#include <sched/process.h>

extern list_t g_sched_processes;
extern slock_t g_sched_threads_all_lock;
extern list_t g_sched_threads_all;
extern list_t g_sched_threads_queued;

/**
 * @brief Create a process
 */
process_t *sched_process_create();

/**
 * @brief Schedule a thread
 *
 * @param thread Thread to be scheduled
 */
void sched_thread_schedule(thread_t *thread);

/**
 * @brief Retrieve the next thread for scheduling
 *
 * @return Thread ready for scheduling
 */
thread_t *sched_thread_next();

/**
 * @brief Called when a thread is dropped by a CPU
 *
 * @param thread Thread
 */
void sched_thread_drop(thread_t *thread);