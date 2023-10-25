#pragma once
#include <stdint.h>
#include <lib/auxv.h>
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
 * @param ip Instruction pointer
 * @param sp Stack pointer
 * @return New thread
 */
thread_t *arch_sched_thread_create_user(process_t *proc, uintptr_t ip, uintptr_t sp);

/**
 * @brief Setup a new user stack
 *
 * @param proc Process
 * @param argv Argument list, NULL terminated
 * @param envp Environment list, NULL terminated
 * @param auxv Auxiliary vector, NULL terminated
 * @return Stack pointer to the new stack
 */
uintptr_t arch_sched_stack_setup(process_t *proc, char **argv, char **envp, auxv_t *auxv);

/**
 * @brief Returns the active thread on the current CPU
 *
 * @return Current thread
 */
thread_t *arch_sched_thread_current();