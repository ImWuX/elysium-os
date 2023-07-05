#ifndef ARCH_AMD64_SCHED_H
#define ARCH_AMD64_SCHED_H

#include <sys/sched.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/msr.h>

extern uint8_t g_sched_vector;

/**
 * @brief Scheduler entry from interrupt
 * 
 * @param frame CPU state at the time of interrupt
 */
void sched_entry(interrupt_frame_t *frame);

/**
 * @brief Retrieve the current CPUs thread
 * 
 * @return Current thread
 */
static inline sched_thread_t *sched_get_current_thread() {
    sched_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return thread;
}

/**
 * @brief Set the current CPUs thread
 * 
 * @param thread Current thread
 */
static inline void sched_set_current_thread(sched_thread_t *thread) {
    msr_write(MSR_GS_BASE, (uint64_t) thread);
}

#endif