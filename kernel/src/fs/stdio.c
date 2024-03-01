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
    (*attr).file_size = 0;
    return 0;
}

static int stdio_stdin_node_rw(vfs_node_t *file [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_WRITE) return -EPERM;
    // TODO: STDIN
    *rw_count = 0;
    return 0;
}

static int stdio_stdout_node_rw(vfs_node_t *file [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_READ) return -EPERM;
    char *c = (char *) packet->buffer;
    for(; *c != 0; c++) log_raw(*c);
    *rw_count = packet->size;
    return 0;
}

static int stdio_stderr_node_rw(vfs_node_t *file [[maybe_unused]], vfs_rw_t *packet, size_t *rw_count) {
    if(packet->rw == VFS_RW_READ) return -EPERM;
    char *c = (char *) packet->buffer;
    for(; *c != 0; c++) log_raw(*c);
    *rw_count = packet->size;
    return 0;
}

static int stdio_shared_node_lookup(vfs_node_t *dir [[maybe_unused]], char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_readdir(vfs_node_t *dir [[maybe_unused]], int *offset [[maybe_unused]], char **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_mkdir(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static int stdio_shared_node_create(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -ENOTDIR;
}

static vfs_node_ops_t stdin_ops = {
    .attr = stdio_shared_node_attr,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stdin_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create
};

static vfs_node_ops_t stdout_ops = {
    .attr = stdio_shared_node_attr,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stdout_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create
};

static vfs_node_ops_t stderr_ops = {
    .attr = stdio_shared_node_attr,
    .lookup = stdio_shared_node_lookup,
    .rw = stdio_stderr_node_rw,
    .mkdir = stdio_shared_node_mkdir,
    .readdir = stdio_shared_node_readdir,
    .create = stdio_shared_node_create
};

static int stdio_root_node_attr(vfs_node_t *node [[maybe_unused]], vfs_node_attr_t *attr) {
    (*attr).file_size = 0;
    return 0;
}

static int stdio_root_node_lookup(vfs_node_t *dir, char *name, vfs_node_t **out) {
    if(strcmp(name, ".") == 0) {
        *out = dir;
        return 0;
    }
    if(strcmp(name, "stdin") == 0) {
        *out = NODES(dir->vfs)->stdin;
        return 0;
    }
    if(strcmp(name, "stdout") == 0) {
        *out = NODES(dir->vfs)->stdout;
        return 0;
    }
    if(strcmp(name, "stderr") == 0) {
        *out = NODES(dir->vfs)->stderr;
        return 0;
    }
    return -ENOENT;
}

static int stdio_root_node_rw(vfs_node_t *file [[maybe_unused]], vfs_rw_t *packet [[maybe_unused]], size_t *rw_count [[maybe_unused]]) {
    return -EISDIR;
}

static int stdio_root_node_readdir(vfs_node_t *dir [[maybe_unused]], int *offset, char **out) {
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

static int stdio_root_node_mkdir(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EROFS;
}

static int stdio_root_node_create(vfs_node_t *parent [[maybe_unused]], const char *name [[maybe_unused]], vfs_node_t **out [[maybe_unused]]) {
    return -EROFS;
}

static vfs_node_ops_t root_ops = {
    .attr = stdio_root_node_attr,
    .lookup = stdio_root_node_lookup,
    .rw = stdio_root_node_rw,
    .mkdir = stdio_root_node_mkdir,
    .readdir = stdio_root_node_readdir,
    .create = stdio_root_node_create
};

static int stdio_mount(vfs_t *vfs, [[maybe_unused]] void *data) {
    stdio_nodes_t *nodes = heap_alloc(sizeof(stdio_nodes_t));

    nodes->root = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->root, 0, sizeof(vfs_node_t));
    nodes->root->vfs = vfs;
    nodes->root->type = VFS_NODE_TYPE_DIR;
    nodes->root->ops = &root_ops;

    nodes->stdin = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stdin, 0, sizeof(vfs_node_t));
    nodes->stdin->vfs = vfs;
    nodes->stdin->type = VFS_NODE_TYPE_FILE;
    nodes->stdin->ops = &stdin_ops;

    nodes->stdout = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stdout, 0, sizeof(vfs_node_t));
    nodes->stdout->vfs = vfs;
    nodes->stdout->type = VFS_NODE_TYPE_FILE;
    nodes->stdout->ops = &stdout_ops;

    nodes->stderr = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->stderr, 0, sizeof(vfs_node_t));
    nodes->stderr->vfs = vfs;
    nodes->stderr->type = VFS_NODE_TYPE_FILE;
    nodes->stderr->ops = &stderr_ops;

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