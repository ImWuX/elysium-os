#include "vfs.h"
#include <string.h>
#include <lib/assert.h>
#include <lib/panic.h>
#include <memory/heap.h>

list_t g_vfs_all = LIST_INIT;

int vfs_mount(vfs_ops_t *vfs_ops, char *path, void *data) {
    vfs_t *vfs = heap_alloc(sizeof(vfs_t));
    vfs->ops = vfs_ops;
    vfs_ops->mount(vfs, data);
    // TODO: Actually mount on the path
    list_insert_behind(&g_vfs_all, &vfs->list);
    return 0;
}

int vfs_lookup(char *path, vfs_node_t **out, vfs_context_t *context) {
    ASSERT(g_vfs_all.next);
    vfs_t *vfs = LIST_GET(g_vfs_all.next, vfs_t, list);

    vfs_node_t *current_node = context->cwd;
    int comp_start = 0, comp_end = 0;
    if(path[comp_end] == '/') {
        int r = vfs->ops->root(vfs, &current_node);
        if(r < 0) return r;
        comp_end++, comp_start++;
    }
    do {
        switch(path[comp_end]) {
            case 0:
            case '/':
                if(comp_start == comp_end) {
                    comp_start++;
                    break;
                }
                int comp_length = comp_end - comp_start;
                char *component = heap_alloc(comp_length + 1);
                memcpy(component, path + comp_start, comp_length);
                component[comp_length] = 0;
                comp_start = comp_end + 1;

                int r = current_node->ops->lookup(current_node, component, &current_node);
                heap_free(component);
                if(r < 0) return r;
                break;
        }
    } while(path[comp_end++]);
    *out = current_node;
    return 0;
}

int vfs_rw(char *path, vfs_rw_t *packet, size_t *rw_count, vfs_context_t *context) {
    vfs_node_t *file;
    int r = vfs_lookup(path, &file, context);
    if(r < 0) return r;
    return file->ops->rw(file, packet, rw_count);
}

int vfs_mkdir(char *path, const char *name, vfs_node_t **out, vfs_context_t *context) {
    vfs_node_t *parent;
    int r = vfs_lookup(path, &parent, context);
    if(r < 0) return r;
    return parent->ops->mkdir(parent, name, out);
}

int vfs_create(char *path, const char *name, vfs_node_t **out, vfs_context_t *context) {
    vfs_node_t *parent;
    int r = vfs_lookup(path, &parent, context);
    if(r < 0) return r;
    return parent->ops->create(parent, name, out);
}