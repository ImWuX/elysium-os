#include "resource.h"
#include <errno.h>
#include <common/log.h>
#include <common/spinlock.h>
#include <memory/heap.h>

resource_t *resource_create_at(resource_table_t *table, int id, vfs_node_t *node, size_t offset, resource_mode_t mode, bool lock) {
    if(lock) spinlock_acquire(&table->lock);
    resource_t *resource = heap_alloc(sizeof(resource_t));
    resource->node = node;
    resource->offset = offset;
    resource->mode = mode;
    table->resources[id] = resource;
    if(lock) spinlock_release(&table->lock);
    log(LOG_LEVEL_DEBUG, "RESOURCE", "Created resource %i (offset: %#lx, mode: %i)", id, offset, mode);
    return resource;
}

int resource_create(resource_table_t *table, vfs_node_t *node, size_t offset, resource_mode_t mode) {
    spinlock_acquire(&table->lock);
    bool found = false;
    int id = 0;
    for(; id < table->count; id++) {
        if(table->resources[id] != NULL) continue;
        found = true;
        break;
    }
    if(!found) {
        spinlock_release(&table->lock);
        return -EMFILE;
    }
    resource_create_at(table, id, node, offset, mode, false);
    spinlock_release(&table->lock);
    return id;
}

int resource_remove(resource_table_t *table, int id) {
    spinlock_acquire(&table->lock);
    if(table->resources[id] == NULL) {
        spinlock_release(&table->lock);
        return -EBADF;
    }
    heap_free(table->resources[id]);
    table->resources[id] = NULL;
    spinlock_release(&table->lock);
    return 0;
}

resource_t *resource_get(resource_table_t *table, int id) {
    if(id < 0 || id >= table->count) return NULL;
    resource_t *resource = NULL;
    spinlock_acquire(&table->lock);
    if(table->resources[id] != NULL) resource = table->resources[id];
    spinlock_release(&table->lock);
    return resource;
}