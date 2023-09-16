#pragma once
#include <stdint.h>
#include <lib/container.h>
#include <sys/cpu.h>
#include <arch/amd64/tss.h>

#define ARCH_CPU(CPU) (container_of(CPU, arch_cpu_t, common))

typedef struct {
    uint8_t lapic_id;
    uint64_t lapic_timer_frequency;
    tss_t *tss;
    cpu_t common;
} arch_cpu_t;