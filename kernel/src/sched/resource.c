#include "resource.h"
#include <errno.h>
#include <common/spinlock.h>
#include <memory/heap.h>

resource_t *resource_create_at(resource_table_t *table, vfs_node_t *node, int id, bool lock) {
    if(lock) spinlock_acquire(&table->lock);
    resource_t *resource = heap_alloc(sizeof(resource_t));
    resource->node = node;
    resource->refs = 1;
    resource->offset = 0;
    table->resources[id] = resource;
    if(lock) spinlock_release(&table->lock);
    return resource;
}

int resource_create(resource_table_t *table, vfs_node_t *node) {
    spinlock_acquire(&table->lock);
    bool found = false;
    int id = 0;
    for(; id < table->count; id++) {
        if(table->resources[id] == NULL) continue;
        found = true;
        break;
    }
    if(!found) {
        spinlock_release(&table->lock);
        return -EMFILE;
    }
    resource_create_at(table, node, id, false);
    spinlock_release(&table->lock);
    return id;
}

resource_t *resource_get(resource_table_t *table, int id) {
    if(id < 0 || id >= table->count) return NULL;
    resource_t *resource = NULL;
    spinlock_acquire(&table->lock);
    if(table->resources[id] != NULL) resource = table->resources[id];
    spinlock_release(&table->lock);
    return resource;
}