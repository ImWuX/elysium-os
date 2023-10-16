#pragma once
#include <stdint.h>
#include <stddef.h>
#include <memory/vmm.h>

/* @warning Assumes params are valid */
vmm_segment_t *seg_direct(vmm_address_space_t *as, uintptr_t base, size_t length, int prot, uintptr_t paddr);