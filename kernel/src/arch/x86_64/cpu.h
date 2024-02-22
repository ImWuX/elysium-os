#pragma once
#include <lib/container.h>
#include <sys/cpu.h>
#include <arch/x86_64/tss.h>

#define X86_64_CPU(CPU) (CONTAINER_OF((CPU), x86_64_cpu_t, common))

typedef struct x86_64_cpu {
    uint32_t lapic_id;
    uint64_t lapic_timer_frequency;

    x86_64_tss_t *tss;

    uintptr_t tlb_shootdown_cr3;
    spinlock_t tlb_shootdown_check;
    spinlock_t tlb_shootdown_lock;

    cpu_t common;
} x86_64_cpu_t;