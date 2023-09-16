#pragma once
#include <sched/thread.h>

/**
 * @brief Destroys a thread
 * @warning Thread should not be on the scheduler queue when this is called
 *
 * @param thread Thread
 */
void arch_sched_thread_destroy(thread_t *thread);

/**
 * @brief Creates a new kernel thread
 *
 * @param func Thread function
 * @return New thread
 */
thread_t *arch_sched_thread_create_kernel(void (* func)());

/**
 * @brief Returns the active thread on the current CPU
 *
 * @return Current thread
 */
thread_t *arch_sched_thread_current();