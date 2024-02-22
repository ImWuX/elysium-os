#pragma once
#include <sched/thread.h>

/**
 * @brief Destroys a thread
 * @warning Thread should not be on the scheduler queue when this is called,
 * set thread state to `THREAD_STATE_DESTROY` if you want to destroy a thread
 */
void arch_sched_thread_destroy(thread_t *thread);

/**
 * @brief Creates a new kernel thread
 * @param func Thread function
 */
thread_t *arch_sched_thread_create_kernel(void (* func)());

/**
 * @brief Returns the active thread on the current CPU
 */
thread_t *arch_sched_thread_current();