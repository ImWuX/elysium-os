#pragma once
#include <lib/list.h>
#include <common/spinlock.h>
#include <memory/vmm.h>

typedef struct {
    long id;
    spinlock_t lock;
    vmm_address_space_t *address_space;
    list_t threads;
    list_element_t list_sched;
} process_t;