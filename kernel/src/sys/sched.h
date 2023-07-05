#ifndef SYS_SCHED_H
#define SYS_SCHED_H

#include <arch/types.h>
#include <memory/vmm.h>

typedef struct sched_thread {
    struct sched_thread *this;
    uint32_t id;
    int lock;
    arch_cpu_context_t context;
    vmm_address_space_t *address_space;
    arch_cpu_local_t *cpu_local;
    struct sched_thread *next;
} sched_thread_t;

/**
 * @brief Add a thread to the scheduler
 * 
 * @param thread Thread
 */
void sched_add(sched_thread_t *thread);

/**
 * @brief Retrieve the next thread due to be scheduled
 * 
 * @param current_thread Current thread
 * @return Next thread or NULL
 */
sched_thread_t *sched_next_thread(sched_thread_t *current_thread);

#endif