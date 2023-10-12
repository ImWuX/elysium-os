#pragma once
#include <fs/vfs.h>

int rdsk_mount(vfs_t *vfs, void *data);
int rdsk_root(vfs_t *vfs, vfs_node_t **out);

static vfs_ops_t g_rdsk_ops = {
    .mount = &rdsk_mount,
    .root = &rdsk_root
};