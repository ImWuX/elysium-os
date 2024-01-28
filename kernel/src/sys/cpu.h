#pragma once
#include <memory/vmm.h>

typedef struct {
    vmm_address_space_t *address_space;
} cpu_t;