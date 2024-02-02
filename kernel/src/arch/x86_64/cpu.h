#pragma once
#include <stdint.h>
#include <sys/cpu.h>
#include <lib/container.h>
#include <lib/spinlock.h>

#define X86_64_CPU(CPU) (CONTAINER_OF((CPU), x86_64_cpu_t, common))

typedef struct x86_64_cpu {
    struct x86_64_cpu *this; // TODO: this can be removed once gs is set to thread
    uint32_t lapic_id;
    cpu_t common;
    uintptr_t tlb_shootdown_cr3;
    spinlock_t tlb_shootdown_check;
    spinlock_t tlb_shootdown_lock;
} x86_64_cpu_t;

extern x86_64_cpu_t g_x86_64_cpus[];
extern volatile size_t g_x86_64_cpus_initialized;