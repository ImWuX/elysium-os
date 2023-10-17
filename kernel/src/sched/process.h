#pragma once
#include <lib/list.h>
#include <lib/slock.h>
#include <memory/vmm.h>
#include <fs/vfs.h>

#define PROCESS_MAX_FDS 256

typedef struct {
    vfs_node_t *file;
} process_fd_t;

typedef struct {
    long id;
    slock_t lock;
    vmm_address_space_t *address_space;
    list_t threads;
    list_t list_sched;
    process_fd_t *fds[PROCESS_MAX_FDS];
} process_t;