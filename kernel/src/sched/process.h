#pragma once
#include <lib/list.h>
#include <common/spinlock.h>
#include <memory/vmm.h>
#include <sched/resource.h>
#include <fs/vfs.h>

typedef struct process {
    long id;
    spinlock_t lock;
    vmm_address_space_t *address_space;
    vfs_node_t *cwd;
    resource_table_t resource_table;
    list_t threads;
    list_element_t list_sched;
} process_t;