#include "tmpfs.h"
#include <errno.h>
#include <lib/mem.h>
#include <lib/str.h>
#include <common/assert.h>
#include <memory/heap.h>

#define INFO(VFS) ((tmpfs_info_t *) (VFS)->data)
#define TNODE(NODE) ((tmpfs_node_t *) (NODE)->data)

typedef struct {
    struct tmpfs_node *root_dir;
    uint64_t id_counter;
} tmpfs_info_t;

typedef struct {
    uintptr_t base;
    size_t size;
} tmpfs_file_t;

typedef struct tmpfs_node {
    uint64_t id;
    const char *name;
    vfs_node_t *node;
    struct tmpfs_node *parent;
    struct tmpfs_node *sibling_next;
    union {
        struct {
            struct tmpfs_node *children;
        } dir;
        tmpfs_file_t *file;
    };
} tmpfs_node_t;

static vfs_node_ops_t g_node_ops;

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
    tnode->id = INFO(vfs)->id_counter++;
    tnode->parent = parent;
    tnode->name = name;
    if(parent) tnode->sibling_next = parent->dir.children;
    if(parent) parent->dir.children = tnode;
    if(is_dir) tnode->dir.children = 0;
    if(!is_dir) {
        tmpfs_file_t *tfile = heap_alloc(sizeof(tmpfs_file_t));
        memset(tfile, 0, sizeof(tmpfs_file_t));
        tnode->file = tfile;
    }

    vfs_node_t *node = heap_alloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->vfs = vfs;
    node->type = is_dir ? VFS_NODE_TYPE_DIR : VFS_NODE_TYPE_FILE;
    node->data = (void *) tnode;
    node->ops = &g_node_ops;

    tnode->node = node;
    return tnode;
}

static int tmpfs_node_attr(vfs_node_t *node, vfs_node_attr_t *attr) {
    attr->device_id = 0; // TODO: set real device id
    attr->inode = TNODE(node)->id;
    attr->size = 0;
    if(node->type == VFS_NODE_TYPE_FILE) attr->size = TNODE(node)->file->size;
    attr->block_size = 1; // TODO: ensure this is not a terrible way of doing this
    attr->block_count = attr->size;
    return 0;
}

static int tmpfs_node_lookup(vfs_node_t *node, char *name, vfs_node_t **out) {
    if(node->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    if(strcmp(name, ".") == 0) {
        *out = node;
        return 0;
    }
    if(strcmp(name, "..") == 0) {
        tmpfs_node_t *parent = TNODE(node)->parent;
        if(parent)
            *out = parent->node;
        else
            *out = node;
        return 0;
    }
    tmpfs_node_t *tnode = dir_find((tmpfs_node_t *) node->data, name);
    if(!tnode) return -ENOENT;
    *out = tnode->node;
    return 0;
}

static int tmpfs_node_rw(vfs_node_t *node, vfs_rw_t *packet, size_t *rw_count) {
    ASSERT(packet->buffer != NULL);
    if(node->type != VFS_NODE_TYPE_FILE) return -EISDIR; // TODO: This errno for this assertion is not strictly correct
    tmpfs_file_t *tfile = (tmpfs_file_t *) TNODE(node)->file;

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

static int tmpfs_node_readdir(vfs_node_t *node, int *offset, char **out) {
    if(node->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tnode = TNODE(node)->dir.children;
    for(int i = 0; i < *offset && tnode; i++) tnode = tnode->sibling_next;
    if(tnode) *out = (char *) tnode->name;
    else *out = NULL;
    (*offset)++;
    return 0;
}

static int tmpfs_node_mkdir(vfs_node_t *node, const char *name, vfs_node_t **out) {
    if(node->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tparent = TNODE(node);
    if(dir_find(tparent, name)) return -EEXIST;
    tmpfs_node_t *dir = create_tnode(tparent, node->vfs, true, name);
    *out = dir->node;
    return 0;
}

static int tmpfs_node_create(vfs_node_t *node, const char *name, vfs_node_t **out) {
    if(node->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    tmpfs_node_t *tparent = TNODE(node);
    if(dir_find(tparent, name)) return -EEXIST;
    tmpfs_node_t *file = create_tnode(tparent, node->vfs, false, name);
    *out = file->node;
    return 0;
}

static int tmpfs_node_truncate(vfs_node_t *node, size_t length) {
    if(node->type != VFS_NODE_TYPE_FILE) return -EISDIR; // TODO: This errno for this assertion is not strictly correct
    tmpfs_node_t *tnode = TNODE(node);
    void *buf;
    if(length > 0) {
        buf = heap_alloc(length);
        memset(buf, 0, length);
        if((void *) tnode->file->base != NULL) memcpy(buf, (void *) tnode->file->base, tnode->file->size < length ? tnode->file->size : length);
    } else {
        buf = NULL;
    }
    if((void *) tnode->file->base != NULL) heap_free((void *) tnode->file->base);
    tnode->file->base = (uintptr_t) buf;
    tnode->file->size = length;
    return 0;
}

static int tmpfs_mount(vfs_t *vfs, [[maybe_unused]] void *data) {
    tmpfs_info_t *info = heap_alloc(sizeof(tmpfs_info_t));
    info->id_counter = 1;
    vfs->data = (void *) info;
    info->root_dir = create_tnode(NULL, vfs, true, "tmpfs_root");
    return 0;
}

static int tmpfs_root(vfs_t *vfs, vfs_node_t **out) {
    *out = INFO(vfs)->root_dir->node;
    return 0;
}

static vfs_node_ops_t g_node_ops = {
    .attr = tmpfs_node_attr,
    .lookup = tmpfs_node_lookup,
    .rw = tmpfs_node_rw,
    .mkdir = tmpfs_node_mkdir,
    .readdir = tmpfs_node_readdir,
    .create = tmpfs_node_create,
    .truncate = tmpfs_node_truncate
};

vfs_ops_t g_tmpfs_ops = {
    .mount = tmpfs_mount,
    .root = tmpfs_root
};