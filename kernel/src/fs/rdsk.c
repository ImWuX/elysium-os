#include "rdsk.h"
#include <stdint.h>
#include <string.h>
#include <lib/assert.h>
#include <klibc/errno.h>
#include <memory/heap.h>

#define SUPPORTED_REVISION ((1 << 8) | 1)

typedef uint64_t rdsk_index_t;

typedef struct {
    char signature[4];
    uint16_t revision;
    uint16_t header_size;
    uint64_t root_index;
    uint64_t nametable_offset;
    uint64_t nametable_size;
    uint16_t dirtable_entry_size;
    uint64_t dirtable_entry_count;
    uint64_t dirtable_offset;
    uint16_t filetable_entry_size;
    uint64_t filetable_entry_count;
    uint64_t filetable_offset;
} __attribute__((packed)) rdsk_header_t;

typedef struct {
    bool used;
    uint64_t nametable_offset;
    uint64_t data_offset;
    uint64_t size;
    uint64_t next_index;
    uint64_t parent_index;
} __attribute__((packed)) rdsk_file_t;

typedef struct {
    bool used;
    uint64_t nametable_offset;
    uint64_t filetable_index;
    uint64_t dirtable_index;
    uint64_t next_index;
    uint64_t parent_index;
} __attribute__((packed)) rdsk_dir_t;

typedef struct {
    rdsk_header_t *header;
    vfs_node_t **dir_cache;
    uint64_t dir_cache_size;
    vfs_node_t **file_cache;
    uint64_t file_cache_size;
} info_t;

static vfs_node_ops_t node_ops;

static const char *get_name(vfs_t *vfs, uint64_t offset) {
    info_t *info = (info_t *) vfs->data;
    return (const char *) ((uintptr_t) info->header + info->header->nametable_offset + offset);
}

static rdsk_dir_t *get_dir(vfs_t *vfs, rdsk_index_t index) {
    info_t *info = (info_t *) vfs->data;
    return (rdsk_dir_t *) ((uintptr_t) info->header + info->header->dirtable_offset + (index - 1) * info->header->dirtable_entry_size);
}

static rdsk_file_t *get_file(vfs_t *vfs, rdsk_index_t index) {
    info_t *info = (info_t *) vfs->data;
    return (rdsk_file_t *) ((uintptr_t) info->header + info->header->filetable_offset + (index - 1) * info->header->filetable_entry_size);
}

static rdsk_index_t get_dir_index(vfs_t *vfs, rdsk_dir_t *dir) {
    info_t *info = (info_t *) vfs->data;
    return ((uintptr_t) dir - ((uintptr_t) info->header + info->header->dirtable_offset)) / info->header->dirtable_entry_size + 1;
}

static rdsk_index_t get_file_index(vfs_t *vfs, rdsk_file_t *file) {
    info_t *info = (info_t *) vfs->data;
    return ((uintptr_t) file - ((uintptr_t) info->header + info->header->filetable_offset)) / info->header->filetable_entry_size + 1;
}

static vfs_node_t *get_dir_vfs_node(vfs_t *vfs, rdsk_index_t index) {
    info_t *info = (info_t *) vfs->data;
    if(info->dir_cache[index - 1]) return info->dir_cache[index - 1];
    vfs_node_t *node = heap_alloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->vfs = vfs;
    node->type = VFS_NODE_TYPE_DIR;
    node->data = get_dir(vfs, index);
    node->ops = &node_ops;
    return node;
}

static vfs_node_t *get_file_vfs_node(vfs_t *vfs, rdsk_index_t index) {
    info_t *info = (info_t *) vfs->data;
    if(info->file_cache[index - 1]) return info->file_cache[index - 1];
    vfs_node_t *node = heap_alloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->vfs = vfs;
    node->type = VFS_NODE_TYPE_FILE;
    node->data = get_file(vfs, index);
    node->ops = &node_ops;
    return node;
}

static rdsk_dir_t *find_dir(vfs_t *vfs, rdsk_dir_t *dir, char *name) {
    rdsk_index_t curindex = dir->dirtable_index;
    while(curindex != 0) {
        rdsk_dir_t *curdir = get_dir(vfs, curindex);
        curindex = curdir->next_index;
        if(strcmp(name, get_name(vfs, curdir->nametable_offset)) != 0) continue;
        return curdir;
    }
    return NULL;
}

static rdsk_file_t *find_file(vfs_t *vfs, rdsk_dir_t *dir, char *name) {
    rdsk_index_t curindex = dir->filetable_index;
    while(curindex != 0) {
        rdsk_file_t *curfile = get_file(vfs, curindex);
        curindex = curfile->next_index;
        if(strcmp(name, get_name(vfs, curfile->nametable_offset)) != 0) continue;
        return curfile;
    }
    return NULL;
}

static int rdsk_node_attr(vfs_node_t *node, vfs_node_attr_t *attr) {
    if(node->type == VFS_NODE_TYPE_FILE) {
        attr->file_size = ((rdsk_file_t *) node->data)->size;
    } else {
        attr->file_size = 0;
    }
    return 0;
}

static int rdsk_node_lookup(vfs_node_t *dir, char *name, vfs_node_t **out) {
    if(dir->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    if(strcmp(name, ".") == 0) {
        *out = dir;
        return 0;
    }
    rdsk_dir_t *rdir = (rdsk_dir_t *) dir->data;
    if(strcmp(name, "..") == 0) {
        if(rdir->parent_index)
            *out = get_dir_vfs_node(dir->vfs, rdir->parent_index);
        else
            *out = dir;
        return 0;
    }
    rdsk_file_t *ffile = find_file(dir->vfs, rdir, name);
    if(ffile) {
        *out = get_file_vfs_node(dir->vfs, get_file_index(dir->vfs, ffile));
        return 0;
    }
    rdsk_dir_t *fdir = find_dir(dir->vfs, rdir, name);
    if(fdir) {
        *out = get_dir_vfs_node(dir->vfs, get_dir_index(dir->vfs, fdir));
        return 0;
    }
    return -ENOENT;
}

static int rdsk_node_rw(vfs_node_t *file, vfs_rw_t *packet, size_t *rw_count) {
    ASSERT(packet->buffer);
    if(file->type != VFS_NODE_TYPE_FILE) return -EISDIR; // TODO: This errno for this assertion is not strictly correct
    if(packet->rw == VFS_RW_WRITE) return -EROFS;

    info_t *info = (info_t *) file->vfs->data;
    rdsk_file_t *rfile = (rdsk_file_t *) file->data;
    if(packet->offset >= rfile->size) {
        *rw_count = 0;
        return 0;
    }
    size_t count = rfile->size - packet->offset;
    if(count > packet->size) count = packet->size;
    memcpy(packet->buffer, (void *) (((uintptr_t) info->header + rfile->data_offset) + packet->offset), count);
    *rw_count = count;
    return 0;
}

static int rdsk_node_readdir(vfs_node_t *dir, int *offset, char **out) {
    if(dir->type != VFS_NODE_TYPE_DIR) return -ENOTDIR;
    int offsetcp = *offset;

    info_t *info = (info_t *) dir->vfs->data;
    rdsk_dir_t *rdir = (rdsk_dir_t *) dir->data;

    rdsk_index_t curindex = rdir->filetable_index;
    for(int i = 0; i < offsetcp && curindex; i++) curindex = get_file(dir->vfs, curindex)->next_index;
    if(curindex) {
        *out = (char *) get_name(dir->vfs, get_file(dir->vfs, curindex)->nametable_offset);
        (*offset)++;
        return 0;
    }

    offsetcp -= info->header->filetable_entry_count; // TODO: -1 ??
    curindex = rdir->dirtable_index;
    for(int i = 0; i < offsetcp; i++) curindex = get_dir(dir->vfs, curindex)->next_index;

    if(curindex) {
        *out = (char *) get_name(dir->vfs, get_dir(dir->vfs, curindex)->nametable_offset);
    } else {
        *out = 0;
    }
    (*offset)++;
    return 0;
}

static int rdsk_node_mkdir(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EROFS;
}

static int rdsk_node_create(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EROFS;
}

int rdsk_mount(vfs_t *vfs, void *data) {
    rdsk_header_t *header = (rdsk_header_t *) data;
    if(header->revision > SUPPORTED_REVISION) return -EINVAL;
    info_t *info = heap_alloc(sizeof(info_t));
    info->header = header;
    info->dir_cache_size = header->dirtable_entry_count;
    info->file_cache_size = header->filetable_entry_count;
    info->dir_cache = heap_alloc(sizeof(vfs_node_t *) * info->dir_cache_size);
    memset(info->dir_cache, 0, sizeof(vfs_node_t *) * info->dir_cache_size);
    info->file_cache = heap_alloc(sizeof(vfs_node_t *) * info->file_cache_size);
    memset(info->file_cache, 0, sizeof(vfs_node_t *) * info->file_cache_size);
    vfs->data = (void *) info;
    return 0;
}

int rdsk_root(vfs_t *vfs, vfs_node_t **out) {
    *out = get_dir_vfs_node(vfs, ((info_t *) vfs->data)->header->root_index);
    return 0;
}

static vfs_node_ops_t node_ops = {
    .attr = &rdsk_node_attr,
    .lookup = &rdsk_node_lookup,
    .rw = &rdsk_node_rw,
    .mkdir = &rdsk_node_mkdir,
    .readdir = &rdsk_node_readdir,
    .create = &rdsk_node_create
};