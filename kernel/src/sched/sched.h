#pragma once
#include <memory/vmm.h>
#include <sched/thread.h>
#include <sched/process.h>

/**
 * @brief Create a process
 * @param address_space
 * @returns new process
 */
process_t *sched_process_create(vmm_address_space_t *address_space);

/**
 * @brief Schedule a thread
 * @param thread
 */
void sched_thread_schedule(thread_t *thread);

/**
 * @brief Retrieve the next thread for execution
 * @return thread ready for execution
 */
thread_t *sched_thread_next();

/**
 * @brief Called when a thread is dropped by a CPU
 * @param thread
 */
void sched_thread_drop(thread_t *thread);