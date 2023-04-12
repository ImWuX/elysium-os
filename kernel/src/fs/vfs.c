#include "vfs.h"
#include <string.h>

#define MAX_PATH 255

vfs_t *g_vfs;

void vfs_initialize(vfs_t *root_vfs) {
    g_vfs = root_vfs;
}

int vfs_lookup(vfs_node_t *cwd, const char *path, vfs_node_t **out) {
    char buffer[MAX_PATH + 1];
    int sub = 0;

    vfs_node_t *node = cwd;
    if(!cwd || path[0] == '/') {
        int ret = g_vfs->ops->root(g_vfs, &node);
        if(ret < 0) return ret;
        sub++;
    }

    for(int i = 0; i < MAX_PATH; i++) {
        if(path[i] == '/' || path[i] == 0) {
            if(sub < i && (i - sub != 1 || path[i - 1] != '.')) {
                memcpy(buffer, path + sub, i - sub);
                buffer[i - sub] = 0;
                int ret = node->ops->lookup(node, &node, buffer);
                if(ret < 0) return ret;
            }
            sub = i + 1;
        }
        if(path[i] == 0) break;
    }

    *out = node;
    return 0;
}