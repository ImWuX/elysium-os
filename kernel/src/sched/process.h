#pragma once
#include <lib/list.h>
#include <lib/slock.h>
#include <memory/vmm.h>
#include <fs/vfs.h>

#define PROCESS_MAX_FDS 256

typedef struct {
    int refs;
    vfs_node_t *node;
    size_t offset;
} process_resource_t;

typedef struct {
    long id;
    slock_t lock;
    vmm_address_space_t *address_space;
    list_t threads;
    list_t list_sched;
    process_resource_t *fds[PROCESS_MAX_FDS];
} process_t;

/**
 * @brief Create a new resource
 * @todo This is a case where we most certainly will need to refactor. actual "resource" structs should be created in a global cache, and the fd numbers allocated per proc...
 * 
 * @param proc Process
 * @param node Node
 * @return File descriptor
 */
int process_resource_create(process_t *proc, vfs_node_t *node);

/**
 * @brief Create a new resource on a specific fd
 * @todo This is a case where we most certainly will need to refactor. actual "resource" structs should be created in a global cache, and the fd numbers allocated per proc...
 * @warning Does not check if fd is free, will override
 *
 * @param proc Process
 * @param node Node
 * @param fd File descriptor
 * @param lock Lock - true = aquire lock
 * @return Resource
 * */
process_resource_t *process_resource_create_at(process_t *proc, vfs_node_t *node, int fd, bool lock);

/**
 * @brief Get a resource by fd
 *
 * @param proc Process
 * @param fd File descriptor
 * @return Resource or NULL
 */
process_resource_t *process_resource_get(process_t *proc, int fd);