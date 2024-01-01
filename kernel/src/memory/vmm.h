#pragma once
#include <lib/spinlock.h>

typedef struct {
    spinlock_t lock;
} vmm_address_space_t;

extern vmm_address_space_t *g_vmm_kernel_address_space;