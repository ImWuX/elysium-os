#ifndef SYS_SCHED_H
#define SYS_SCHED_H

#include <arch/types.h>
#include <memory/vmm.h>
#include <lib/slock.h>
#include <lib/list.h>

typedef enum {
    SCHED_THREAD_ACTIVE,
    SCHED_THREAD_READY,
    SCHED_THREAD_BLOCK,
    SCHED_THREAD_EXIT
} sched_thread_state_t;

typedef struct sched_thread {
    struct sched_thread *this;
    uint32_t id;
    uint8_t priority;
    sched_thread_state_t state;
    arch_cpu_local_t *cpu_local;
    arch_cpu_context_t context;
    vmm_address_space_t *address_space;
    list_t list;
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
 * @return Next thread or NULL
 */
sched_thread_t *sched_next_thread();

/**
 * @brief Schedule a thread
 *
 * @param thread Thread
 */
void sched_schedule_thread(sched_thread_t *thread);

#endif