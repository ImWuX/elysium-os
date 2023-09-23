#include "tmpfs.h"
#include <string.h>
#include <klibc/errno.h>
#include <lib/slock.h>
#include <lib/assert.h>
#include <memory/heap.h>

typedef struct {
    slock_t lock;
    struct tmpfs_node *root_dir;
} tmpfs_info_t;

typedef struct {
    uintptr_t base;
    size_t size;
} tmpfs_file_t;

typedef struct tmpfs_node {
    const char *name;
    vfs_node_t *node;
    bool is_dir;
    struct tmpfs_node *parent;
    struct tmpfs_node *sibling_next;
    union {
        struct {
            struct tmpfs_node *children;
        } dir;
        tmpfs_file_t *file;
    };
} tmpfs_node_t;

static vfs_node_ops_t node_ops;

static tmpfs_node_t *dir_find(tmpfs_node_t *dir, const char *name) {
    tmpfs_node_t *node = dir->dir.children;
    while(node) {
        if(strcmp(node->name, name) == 0) return node;
        node = node->sibling_next;
    }
    return 0;
}

static tmpfs_node_t *create_tnode(tmpfs_node_t *parent, vfs_t *vfs, bool is_dir, const char *name) {
    tmpfs_node_t *tnode = heap_alloc(sizeof(tmpfs_node_t));
    memset(tnode, 0, sizeof(tmpfs_node_t));
    tnode->parent = parent;
    tnode->name = name;
    tnode->is_dir = is_dir;
    if(parent) tnode->sibling_next = parent->dir.children;
    if(parent) parent->dir.children = tnode;
    if(is_dir) tnode->dir.children = 0;
    if(!is_dir) {
        tmpfs_file_t *tfile = heap_alloc(sizeof(tmpfs_file_t));
        memset(tfile, 0, sizeof(tmpfs_file_t));
        tnode->file = tfile;
    }

    vfs_node_t *node = heap_alloc(sizeof(vfs_node_t));
    node->vfs = vfs;
    node->type = is_dir ? VFS_NODE_TYPE_DIR : VFS_NODE_TYPE_FILE;
    node->data = (void *) tnode;
    node->ops = &node_ops;

    tnode->node = node;
    return tnode;
}

static int tmpfs_node_attr(vfs_node_t *node, vfs_node_attr_t *attr) {
    if(node->type == VFS_NODE_TYPE_FILE)
        attr->file_size = ((tmpfs_node_t *) node->data)->file->size;
    else
        attr->file_size = 0;
    return 0;
}

static int tmpfs_node_lookup(vfs_node_t *dir, char *name, vfs_node_t **out) {
    if(dir->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tnode = dir_find((tmpfs_node_t *) dir->data, name);
    if(!tnode) return -ENOENT;
    *out = tnode->node;
    return 0;
}

static int tmpfs_node_rw(vfs_node_t *file, vfs_rw_t *packet, size_t *rw_count) {
    ASSERT(packet->buffer);
    if(file->type != VFS_NODE_TYPE_FILE) return -EISDIR; // TODO: This errno for this assertion is not strictly correct
    tmpfs_file_t *tfile = (tmpfs_file_t *) ((tmpfs_node_t *) file->data)->file;

    *rw_count = 0;
    switch(packet->rw) {
        case VFS_RW_READ:
            if(packet->offset >= tfile->size) return 0;
            size_t count = tfile->size - packet->offset;
            if(count > packet->size) count = packet->size;
            memcpy(packet->buffer, (void *) (tfile->base + packet->offset), count);
            *rw_count = count;
            return 0;
        case VFS_RW_WRITE:
            if(packet->offset + packet->size > tfile->size) {
                void *new_buf = heap_alloc(packet->offset + packet->size);
                if(tfile->base) {
                    memcpy(new_buf, (void *) tfile->base, tfile->size);
                    heap_free((void *) tfile->base);
                }
                tfile->base = (uintptr_t) new_buf;
                tfile->size = packet->offset + packet->size;
            }
            memcpy((void *) (tfile->base + packet->offset), packet->buffer, packet->size);
            *rw_count = packet->size;
            return 0;
    }
    return 0;
}

static int tmpfs_node_readdir(vfs_node_t *dir, int *offset, char **out) {
    if(dir->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tnode = ((tmpfs_node_t *) dir->data)->dir.children;
    for(int i = 0; i < *offset && tnode; i++) tnode = tnode->sibling_next;
    if(tnode) *out = (char *) tnode->name;
    else *out = 0;
    (*offset)++;
    return 0;
}

static int tmpfs_node_mkdir(vfs_node_t *parent, const char *name, vfs_node_t **out) {
    if(parent->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tparent = (tmpfs_node_t *) parent->data;
    if(dir_find(tparent, name)) return -EEXIST;
    tmpfs_node_t *dir = create_tnode(tparent, parent->vfs, true, name);
    *out = dir->node;
    return 0;
}

static int tmpfs_node_create(vfs_node_t *parent, const char *name, vfs_node_t **out) {
    if(parent->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tparent = (tmpfs_node_t *) parent->data;
    if(dir_find(tparent, name)) return -EEXIST;
    tmpfs_node_t *file = create_tnode(tparent, parent->vfs, false, name);
    *out = file->node;
    return 0;
}

int tmpfs_mount(vfs_t *vfs, [[maybe_unused]] void *data) {
    tmpfs_info_t *info = heap_alloc(sizeof(tmpfs_info_t));
    info->lock = SLOCK_INIT;
    info->root_dir = create_tnode(0, vfs, true, "tmpfs_root");
    vfs->data = (void *) info;
    return 0;
}

int tmpfs_root(vfs_t *vfs, vfs_node_t **out) {
    tmpfs_info_t *info = (tmpfs_info_t *) vfs->data;
    *out = info->root_dir->node;
    return 0;
}

static vfs_node_ops_t node_ops = {
    .attr = &tmpfs_node_attr,
    .lookup = &tmpfs_node_lookup,
    .rw = &tmpfs_node_rw,
    .mkdir = &tmpfs_node_mkdir,
    .readdir = &tmpfs_node_readdir,
    .create = &tmpfs_node_create
};