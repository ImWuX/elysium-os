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

#endif