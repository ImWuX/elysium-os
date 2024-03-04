#pragma once
#include <stddef.h>
#include <fs/vfs.h>
#include <common/spinlock.h>

typedef enum {
    RESOURCE_MODE_READ_ONLY,
    RESOURCE_MODE_WRITE_ONLY,
    RESOURCE_MODE_READ_WRITE,
    RESOURCE_MODE_REFERENCE
} resource_mode_t;

typedef struct resource {
    vfs_node_t *node;
    size_t offset;
    resource_mode_t mode;
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
int resource_create(resource_table_t *table, vfs_node_t *node, size_t offset, resource_mode_t mode);

/**
 * @brief Create a resource at id
 * @param id resource id
 * @param lock acquire process lock
 */
resource_t *resource_create_at(resource_table_t *table, int id, vfs_node_t *node, size_t offset, resource_mode_t mode, bool lock);

/**
 * @brief Removes a resource from the table
 * @param id resource id
 * @returns -errno on failure, 0 on success
 */
int resource_remove(resource_table_t *table, int id);

/**
 * @brief Retrieve a resource from a process
 * @param id resource id
 */
resource_t *resource_get(resource_table_t *table, int id);