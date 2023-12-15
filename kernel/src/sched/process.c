#include "process.h"
#include <lib/c/errno.h>
#include <memory/heap.h>

int process_resource_create(process_t *proc, vfs_node_t *node) {
    slock_acquire(&proc->lock);
    bool found = false;
    int fd = 0;
    for(; fd < PROCESS_MAX_FDS; fd++) {
        if(proc->fds[fd]) continue;
        found = true;
        break;
    }
    if(!found) {
        slock_release(&proc->lock);
        return -EMFILE;
    }
    process_resource_create_at(proc, node, fd, false);
    slock_release(&proc->lock);
    return fd;
}

process_resource_t *process_resource_create_at(process_t *proc, vfs_node_t *node, int fd, bool lock) {
    if(lock) slock_acquire(&proc->lock);
    process_resource_t *res = heap_alloc(sizeof(process_resource_t));
    res->node = node;
    res->refs = 1;
    res->offset = 0;
    proc->fds[fd] = res;
    if(lock) slock_release(&proc->lock);
    return res;
}

process_resource_t *process_resource_get(process_t *proc, int fd) {
    if(fd < 0 || fd >= PROCESS_MAX_FDS) return NULL;
    process_resource_t *res = NULL;
    slock_acquire(&proc->lock);
    if(proc->fds[fd]) res = proc->fds[fd];
    slock_release(&proc->lock);
    return res;
}