#ifndef CPU_MSR_H
#define CPU_MSR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MSR_IA32_PAT = 0x277
} msrs_t;

bool msr_available();
uint64_t msr_get(uint64_t msr);
void msr_set(uint64_t msr, uint64_t value);

#endif