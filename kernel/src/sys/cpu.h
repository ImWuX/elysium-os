#pragma once
#include <sched/thread.h>

typedef struct cpu {
    struct thread *idle_thread;
} cpu_t;

/**
 * @brief Get the current CPU local
*/
cpu_t *cpu_current();