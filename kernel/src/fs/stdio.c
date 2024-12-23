#include "stdio.h"
#include <errno.h>
#include <lib/str.h>
#include <lib/mem.h>
#include <common/log.h>
#include <memory/heap.h>

#define NODES(VFS) ((stdio_nodes_t *) (VFS)->data)

typedef struct {
    vfs_node_t *root;
    vfs_node_t *stdin;
    vfs_node_t *stdout;
    vfs_node_t *stderr;
} stdio_nodes_t;

static int stdio_shared_node_attr(vfs_node_t *node [[maybe_unused]], vfs_node_attr_t *attr) {
    attr->size = 0;
    return 0;
}

static const char *stdio_stdin_node_name(vfs_node_t *node [[maybe_unused]]) {
    return "stdin";
}

static int stdio_stdin_node_rw(vfs_node_t *node [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_WRITE) return -EPERM;
    // TODO: STDIN
    *rw_count = 0;
    return 0;
}

static const char *stdio_stdout_node_name(vfs_node_t *node [[maybe_unused]]) {
    return "stdout";
}

static int stdio_stdout_node_rw(vfs_node_t *node [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_READ) return -EPERM;
    char *c = (char *) packet->buffer;
    for(size_t i = 0; i < packet->size && c[i] != 0; i++) log_raw(c[i]);
    *rw_count = packet->size;
    return 0;
}

static const char *stdio_stderr_node_name(vfs_node_t *node [[maybe_unused]]) {
    return "stderr";
}

static int stdio_stderr_node_rw(vfs_node_t *node [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_READ) return -EPERM;
    char *c = (char *) packet->buffer;
    for(size_t i = 0; i < packet->size && c[i] != 0; i++) log_raw(c[i]);
    *rw_count = packet->size;
    return 0;
}

static int stdio_shared_node_lookup(vfs_node_t *node [[maybe_unused]], char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_readdir(vfs_node_t *node [[maybe_unused]], int *offset [[maybe_unused]], char **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_mkdir(vfs_node_t *node [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_create(vfs_node_t *node [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_truncate(vfs_node_t *node [[maybe_unused]], size_t length [[maybe_unused]]) {
    return -EPERM;
}

static vfs_node_ops_t g_stdin_ops = {
    .attr = stdio_shared_node_attr,
    .name = stdio_stdin_node_name,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stdin_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create,
    .truncate = stdio_shared_node_truncate
};

static vfs_node_ops_t g_stdout_ops = {
    .attr = stdio_shared_node_attr,
    .name = stdio_stdout_node_name,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stdout_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create,
    .truncate = stdio_shared_node_truncate
};

static vfs_node_ops_t g_stderr_ops = {
    .attr = stdio_shared_node_attr,
    .name = stdio_stderr_node_name,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stderr_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create,
    .truncate = stdio_shared_node_truncate
};

static int stdio_root_node_attr(vfs_node_t *node [[maybe_unused]], vfs_node_attr_t *attr) {
    // TODO: these values are not correct
    attr->device_id = 0;
    attr->inode = 0;
    attr->size = 0;
    attr->block_count = 0;
    attr->block_size = 0;
    return 0;
}

static const char *stdio_root_node_name(vfs_node_t *node [[maybe_unused]]) {
    return NULL;
}

static int stdio_root_node_lookup(vfs_node_t *node, char *name, vfs_node_t **out) {
    if(strcmp(name, "..") == 0) {
        *out = NULL;
        return 0;
    }
    if(strcmp(name, ".") == 0) {
        *out = node;
        return 0;
    }
    if(strcmp(name, "stdin") == 0) {
        *out = NODES(node->vfs)->stdin;
        return 0;
    }
    if(strcmp(name, "stdout") == 0) {
        *out = NODES(node->vfs)->stdout;
        return 0;
    }
    if(strcmp(name, "stderr") == 0) {
        *out = NODES(node->vfs)->stderr;
        return 0;
    }
    return -ENOENT;
}

static int stdio_root_node_rw(vfs_node_t *node [[maybe_unused]], vfs_rw_t *packet [[maybe_unused]], size_t *rw_count [[maybe_unused]]) {
    return -EISDIR;
}

static int stdio_root_node_readdir(vfs_node_t *node [[maybe_unused]], int *offset, char **out) {
    switch(*offset) {
        case 0:
            *out = "stdin";
            break;
        case 1:
            *out = "stdout";
            break;
        case 2:
            *out = "stderr";
            break;
        default:
            *out = NULL;
            return 0;
    }
    (*offset)++;
    return 0;
}

static int stdio_root_node_mkdir(vfs_node_t *node [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EPERM;
}

static int stdio_root_node_create(vfs_node_t *node [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EPERM;
}

static int stdio_root_node_truncate(vfs_node_t *node [[maybe_unused]], size_t length [[maybe_unused]]) {
    return -EISDIR;
}

static vfs_node_ops_t g_root_ops = {
    .attr = stdio_root_node_attr,
    .name = stdio_root_node_name,
    .lookup = stdio_root_node_lookup,
    .rw = stdio_root_node_rw,
    .mkdir = stdio_root_node_mkdir,
    .readdir = stdio_root_node_readdir,
    .create = stdio_root_node_create,
    .truncate = stdio_root_node_truncate
};

static int stdio_mount(vfs_t *vfs, [[maybe_unused]] void *data) {
    stdio_nodes_t *nodes = heap_alloc(sizeof(stdio_nodes_t));

    nodes->root = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->root, 0, sizeof(vfs_node_t));
    nodes->root->vfs = vfs;
    nodes->root->type = VFS_NODE_TYPE_DIR;
    nodes->root->ops = &g_root_ops;

    nodes->stdin = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stdin, 0, sizeof(vfs_node_t));
    nodes->stdin->vfs = vfs;
    nodes->stdin->type = VFS_NODE_TYPE_FILE;
    nodes->stdin->ops = &g_stdin_ops;

    nodes->stdout = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stdout, 0, sizeof(vfs_node_t));
    nodes->stdout->vfs = vfs;
    nodes->stdout->type = VFS_NODE_TYPE_FILE;
    nodes->stdout->ops = &g_stdout_ops;

    nodes->stderr = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stderr, 0, sizeof(vfs_node_t));
    nodes->stderr->vfs = vfs;
    nodes->stderr->type = VFS_NODE_TYPE_FILE;
    nodes->stderr->ops = &g_stderr_ops;

    vfs->data = (void *) nodes;
    return 0;
}

static int stdio_root(vfs_t *vfs, vfs_node_t **out) {
    *out = NODES(vfs)->root;
    return 0;
}

vfs_ops_t g_stdio_ops = {
    .mount = stdio_mount,
    .root = stdio_root
};