#include "vmm.h"
#include <stdio.h>

void vmm_initialize(uint64_t pml4_address) {
    printf("PML4 %x\n", pml4_address);
}