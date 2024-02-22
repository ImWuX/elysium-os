#pragma once
#include <sched/thread.h>

/**
 * @brief Returns the active thread on the current CPU
 */
thread_t *arch_sched_thread_current();