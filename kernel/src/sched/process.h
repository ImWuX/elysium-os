#pragma once
#include <lib/list.h>
#include <lib/slock.h>
#include <memory/vmm.h>

typedef struct {
    long id;
    slock_t lock;
    vmm_address_space_t *address_space;
    list_t threads;
    list_t list_sched;
} process_t;