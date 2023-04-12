#ifndef FS_VFS_H
#define FS_VFS_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

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

typedef struct vfs_node {
    vfs_t *vfs;
    vfs_node_type_t type;
    struct vfs_node_operations *ops;
    void *data;
} vfs_node_t;

typedef struct vfs_node_operations {
    int (*lookup)(vfs_node_t *node, vfs_node_t **out, const char *name);
    int (*create)(vfs_node_t *node, vfs_node_t **out, const char *name);
    int (*mkdir)(vfs_node_t *node, vfs_node_t **out, const char *name);
    int (*remove)(vfs_node_t *node, const char *name);
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