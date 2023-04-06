#ifndef PMM_LOWMEM_H
#define PMM_LOWMEM_H

#include <stdint.h>

void pmm_lowmem_initialize(void *bitmap_address, uint64_t size);
void *pmm_lowmem_request(unsigned int count);
void pmm_lowmem_release(void *address, unsigned int count);

#endif