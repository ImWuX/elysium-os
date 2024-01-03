#pragma once
#include <stdint.h>
#include <sys/cpu.h>

#define ARCH_CPU(CPU) (container_of(CPU, arch_cpu_t, common))

typedef struct {
    uint32_t lapic_id;
    cpu_t common;
} arch_cpu_t;