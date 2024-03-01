#pragma once
#include <stddef.h>
#include <fs/vfs.h>
#include <common/spinlock.h>

typedef struct resource {
    unsigned int refs;
    vfs_node_t *node;
    size_t offset;
} resource_t;

typedef struct resource_table {
    resource_t **resources;
    int count;
    spinlock_t lock;
} resource_table_t;

/**
 * @brief Create a resource
 * @returns -errno on failure, resource id on success
 */
int resource_create(resource_table_t *table, vfs_node_t *node);

/**
 * @brief Create a resource at id
 * @param id resource id
 * @param lock acquire process lock
 */
resource_t *resource_create_at(resource_table_t *table, vfs_node_t *node, int id, bool lock);

/**
 * @brief Retrieve a resource from a process
 * @param id resource id
 */
resource_t *resource_get(resource_table_t *table, int id);