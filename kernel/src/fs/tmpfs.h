#ifndef FS_TMPFS_H
#define FS_TMPFS_H

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs.h>

typedef struct tmpfs_node {
    char *name;
    vfs_node_t *vfs_node;
    bool is_dir;
    uint64_t size;
    struct tmpfs_node *parent;
    struct tmpfs_node *next;
    struct tmpfs_node *children;
} tmpfs_node_t;

vfs_t *tmpfs_create();

#endif