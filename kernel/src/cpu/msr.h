#ifndef CPU_MSR_H
#define CPU_MSR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MSR_PAT = 0x277,
    MSR_EFER = 0xC0000080,
    MSR_STAR = 0xC0000081,
    MSR_LSTAR = 0xC0000082,
    MSR_CSTAR = 0xC0000083,
    MSR_SFMASK = 0xC0000084
} msrs_t;

bool msr_available();
uint64_t msr_get(uint64_t msr);
void msr_set(uint64_t msr, uint64_t value);

#endif