#ifndef FS_VFS_H
#define FS_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>

#define VFS_MAX_PATH 255

typedef struct vfs {
    struct vfs_node *mount_node;
    struct vfs_operations *ops;
    struct vfs *next;
    void *data;
} vfs_t;

typedef enum {
    VFS_NODE_TYPE_REGULAR,
    VFS_NODE_TYPE_DIRECTORY,
    VFS_NODE_TYPE_LINK,
} vfs_node_type_t;

typedef struct {
    vfs_node_type_t type;
    uint64_t file_size;
    uint16_t uid;
    uint16_t gid;
    uint64_t block_size;
    uint64_t blocks_count;
    uint16_t time_created;
    uint16_t time_last_modified;
    uint16_t time_last_accessed;
} vfs_node_attributes_t;

typedef struct vfs_node {
    vfs_t *vfs;
    vfs_node_type_t type;
    struct vfs_node_operations *ops;
    void *data;
} vfs_node_t;

typedef struct vfs_node_operations {
    int (*getattr)(vfs_node_t *node, vfs_node_attributes_t *out);
    int (*lookup)(vfs_node_t *node, vfs_node_t **out, const char *name);
    int (*create)(vfs_node_t *node, vfs_node_t **out, const char *name, vfs_node_attributes_t *attributes);
    int (*remove)(vfs_node_t *node, const char *name);
    off_t (*readdir)(vfs_node_t *node, dirent_t *out, off_t offset);
    int (*open)(vfs_node_t *node);
    int (*close)(vfs_node_t *node);
} vfs_node_operations_t;

typedef struct vfs_operations {
    int (*mount)(vfs_t *vfs, const char *path, void *data);
    int (*root)(vfs_t *vfs, vfs_node_t **out);
    int (*vget)(vfs_t *vfs, vfs_node_t **out, ino_t inode);
} vfs_operations_t;

extern vfs_t *g_vfs;

void vfs_initialize(vfs_t *root_vfs);
int vfs_lookup(vfs_node_t *cwd, const char *path, vfs_node_t **out);

#endif