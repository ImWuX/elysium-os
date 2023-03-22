#ifndef FS_VFS_H
#define FS_VFS_H

#include <stdint.h>
#include <stdbool.h>

#define VFS_FD_ERROR UINT64_MAX

typedef uint64_t vfs_fd_t;

typedef struct {
    struct vfs_node *root;
} vfs_block_t;

typedef struct vfs_node {
    vfs_fd_t fd;
    uint16_t name_length;
    uint8_t *name;
    bool is_directory;
    struct vfs_node *parent;
    struct vfs_node *next;
    vfs_block_t *block;
    void *block_data;
} vfs_node_t;

typedef struct {

} vfs_node_operations_t;

void vfs_initialize(vfs_block_t *root_block);

#endif