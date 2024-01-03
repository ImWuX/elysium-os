#pragma once
#include <stdint.h>
#include <sys/cpu.h>
#include <lib/spinlock.h>

#define ARCH_CPU(CPU) (container_of(CPU, arch_cpu_t, common))

typedef struct arch_cpu {
    struct arch_cpu *this; // TODO: this can be removed once gs is set to thread
    uint32_t lapic_id;
    cpu_t common;
    uintptr_t tlb_shootdown_cr3;
    spinlock_t tlb_shootdown_check;
    spinlock_t tlb_shootdown_lock;
} arch_cpu_t;

extern arch_cpu_t *g_cpus;
extern volatile int g_cpus_initialized;