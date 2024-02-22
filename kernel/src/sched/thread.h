#pragma once
#include <sys/cpu.h>

typedef struct thread {
    struct cpu *cpu;
} thread_t;