#pragma once
#include <fs/vfs.h>

int tmpfs_mount(vfs_t *vfs, void *data);
int tmpfs_root(vfs_t *vfs, vfs_node_t **out);

static vfs_ops_t g_tmpfs_ops = {
    .mount = &tmpfs_mount,
    .root = &tmpfs_root
};