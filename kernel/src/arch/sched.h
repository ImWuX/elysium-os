#ifndef ARCH_SCHED_H
#define ARCH_SCHED_H

#include <sys/sched.h>

/**
 * @brief Retrieve the current CPUs thread
 * 
 * @return Current thread
 */
sched_thread_t *arch_sched_get_current_thread();

/**
 * @brief Set the current CPUs thread
 * 
 * @param thread Current thread
 */
void arch_sched_set_current_thread(sched_thread_t *thread);

/**
 * @brief Initialize a kernel thread
 * @warning Does not add it to the scheduler
 * 
 * @param thread Thread
 * @param entry Entry, this is assumed to be inside the kernel
 * @param stack_pages Stack size in pages
 */
void arch_sched_init_kernel_thread(sched_thread_t *thread, void *entry);

#endif