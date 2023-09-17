#pragma once
#include <sched/process.h>
#include <sched/thread.h>
#include <memory/vmm.h>

/**
 * @brief Destroys a thread
 * @warning Thread should not be on the scheduler queue when this is called,
 * set thread state to `THREAD_STATE_DESTROY` if you want to destroy a thread
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
 * @brief Creates a new user thread
 *
 * @param proc Parent process
 * @return New thread
 */
thread_t *arch_sched_thread_create_user(process_t *proc);

/**
 * @brief Create a new process
 *
 * @return New process
 */
process_t *arch_sched_process_create();

/**
 * @brief Returns the active thread on the current CPU
 *
 * @return Current thread
 */
thread_t *arch_sched_thread_current();