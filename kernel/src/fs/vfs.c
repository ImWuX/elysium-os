#include "vfs.h"
#include <errno.h>
#include <lib/mem.h>
#include <lib/str.h>
#include <common/log.h>
#include <common/panic.h>
#include <common/assert.h>
#include <memory/heap.h>

list_t g_vfs_all = LIST_INIT_CIRCULAR(g_vfs_all);

int vfs_mount(vfs_ops_t *vfs_ops, char *path, void *data) {
    vfs_t *vfs = heap_alloc(sizeof(vfs_t));
    memset(vfs, 0, sizeof(vfs_t));
    vfs->ops = vfs_ops;
    vfs_ops->mount(vfs, data);
    if(list_is_empty(&g_vfs_all)) {
        if(path != NULL) {
            heap_free(vfs);
            return -ENOENT;
        }
        vfs->mount_node = NULL;
    } else {
        vfs_node_t *node;
        int r = vfs_lookup(path, &node, NULL);
        if(r != 0) {
            heap_free(vfs);
            return r;
        }
        if(node->vfs_mounted != NULL) {
            heap_free(vfs);
            return -EBUSY;
        }
        node->vfs_mounted = vfs;
        vfs->mount_node = node;
    }
    list_prepend(&g_vfs_all, &vfs->list_elem);
    return 0;
}

int vfs_root(vfs_node_t **out) {
    if(list_is_empty(&g_vfs_all)) return -ENOENT;
    ASSERT(LIST_NEXT(&g_vfs_all) != NULL);
    vfs_t *vfs = LIST_CONTAINER_GET(LIST_NEXT(&g_vfs_all), vfs_t, list_elem);
    return vfs->ops->root(vfs, out);
}

int vfs_lookup_ext(char *path, vfs_node_t **out, vfs_node_t *cwd, vfs_lookup_create_t create, bool exclusive) {
    int comp_start = 0, comp_end = 0;

    vfs_node_t *current_node = cwd;
    if(path[comp_end] == '/') {
        int r = vfs_root(&current_node);
        if(r != 0) return r;
        comp_start++, comp_end++;
    }
    if(current_node == NULL) return -ENOENT;

    do {
        bool last_comp = false;
        switch(path[comp_end]) {
            case 0:
                last_comp = true;
                [[fallthrough]];
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

                vfs_node_t *next_node;
                int r = current_node->ops->lookup(current_node, component, &next_node);
                if(r == 0) {
                    if(next_node == NULL) {
                        if(current_node->vfs->mount_node != NULL) {
                            r = current_node->ops->lookup(current_node, "..", &next_node);
                        }
                    } else {
                        current_node = next_node;
                    }
                }

                if(r == -ENOENT && last_comp) {
                    switch(create) {
                        case VFS_LOOKUP_CREATE_FILE:
                            r = current_node->ops->create(current_node, component, &current_node);
                            break;
                        case VFS_LOOKUP_CREATE_DIR:
                            r = current_node->ops->mkdir(current_node, component, &current_node);
                            break;
                        case VFS_LOOKUP_CREATE_NONE:
                            heap_free(component);
                            break;
                    }
                } else {
                    if(exclusive) r = -EEXIST;
                    heap_free(component);
                }

                if(r != 0) return r;
                break;
        }

        if(current_node->vfs_mounted == NULL) continue;
        int r = current_node->vfs_mounted->ops->root(current_node->vfs_mounted, &current_node);
        if(r != 0) return r;
    } while(path[comp_end++]);

    *out = current_node;
    return 0;
}

int vfs_lookup(char *path, vfs_node_t **out, vfs_node_t *cwd) {
    return vfs_lookup_ext(path, out, cwd, VFS_LOOKUP_CREATE_NONE, false);
}

int vfs_rw(char *path, vfs_rw_t *packet, size_t *rw_count, vfs_node_t *cwd) {
    vfs_node_t *file;
    int r = vfs_lookup(path, &file, cwd);
    if(r < 0) return r;
    return file->ops->rw(file, packet, rw_count);
}

int vfs_mkdir(char *path, const char *name, vfs_node_t **out, vfs_node_t *cwd) {
    vfs_node_t *parent;
    int r = vfs_lookup(path, &parent, cwd);
    if(r < 0) return r;
    return parent->ops->mkdir(parent, name, out);
}

int vfs_create(char *path, const char *name, vfs_node_t **out, vfs_node_t *cwd) {
    vfs_node_t *parent;
    int r = vfs_lookup(path, &parent, cwd);
    if(r < 0) return r;
    return parent->ops->create(parent, name, out);
}

char *vfs_path(vfs_node_t *node) {
    // TODO: use heap_realloc when made
    size_t buffer_size = 0;
    char *buffer = NULL;
    while(true) {
        const char *name = node->ops->name(node);
        if(name == NULL) {
            if(node->vfs->mount_node == NULL) break;
            node = node->vfs->mount_node;
            continue;
        }
        size_t name_length = strlen(name);

        char *new_buffer = heap_alloc(name_length + 1 + buffer_size + 1);
        new_buffer[0] = '/';
        memcpy(&new_buffer[1], name, name_length);
        if(buffer_size != 0) memcpy(&new_buffer[name_length + 1], buffer, buffer_size);
        new_buffer[name_length + 1 + buffer_size] = '\0';

        buffer_size += name_length + 1;
        heap_free(buffer);
        buffer = new_buffer;

        ASSERT(node->ops->lookup(node, "..", &node) == 0);
    }
    if(buffer == NULL) {
        buffer = heap_alloc(2);
        buffer[0] = '/', buffer[1] = '\0';
    }
    return buffer;
}