#pragma once
#include <stddef.h>
#include <stdint.h>
#include <lib/list.h>

typedef struct vfs {
    struct vfs_node *mount_node;
    struct vfs_ops *ops;
    void *data;
    list_t list;
} vfs_t;

typedef enum {
    VFS_NODE_TYPE_FILE,
    VFS_NODE_TYPE_DIR
} vfs_node_type_t;

typedef struct {
    size_t file_size;
} vfs_node_attr_t;

typedef struct vfs_node {
    vfs_t *vfs;
    vfs_t *vfs_mounted;
    struct vfs_node_ops *ops;
    vfs_node_type_t type;
    void *data;
} vfs_node_t;

typedef struct {
    enum {
        VFS_RW_READ,
        VFS_RW_WRITE
    } rw;
    size_t offset;
    size_t size;
    void *buffer;
} vfs_rw_t;

typedef struct {
    vfs_node_t *cwd;
} vfs_context_t;

typedef struct vfs_ops {
    /**
     * @brief Called on VFS mount
     * @param data Private VFS data
     * @returns 0 on success, -errno on failure
     */
    int (* mount)(vfs_t *vfs, void *data);

    /**
     * @brief Retrieves the root node from the VFS
     * @param out root node
     * @returns 0 on success, -errno on failure
     */
    int (* root)(vfs_t *vfs, vfs_node_t **out);
} vfs_ops_t;

typedef struct vfs_node_ops {
    /**
     * @brief Read/write a file
     * @param rw_count bytes read/written (out)
     * @returns 0 on success, -errno on failure
     */
    int (* rw)(vfs_node_t *file, vfs_rw_t *packet, size_t *rw_count);

    /**
     * @brief Retrieve node attributes
     * @param attr attributes struct to populate
     * @returns 0 on success, -errno on failure
     */
    int (* attr)(vfs_node_t *node, vfs_node_attr_t *attr);

    /**
     * @brief Look up a node by name
     * @returns 0 on success, -errno on failure
     */
    int (* lookup)(vfs_node_t *dir, char *name, vfs_node_t **out);

    /**
     * @brief Read the next entry in a directory
     * @param offset pointer to the offset (in & out)
     * @param out directory entry, or NULL
     * @returns 0 on success, -errno on failure
     */
    int (* readdir)(vfs_node_t *dir, int *offset, char **out);

    /**
     * @brief Creates a new directory in a directory
     * @param out new node (directory)
     * @returns 0 on success, -errno on failure
     */
    int (* mkdir)(vfs_node_t *parent, const char *name, vfs_node_t **out);

    /**
     * @brief Creates a new file in a directory
     * @todo Attributes and open mode
     * @param out new node (file)
     * @return 0 on success, -errno on failure
     */
    int (*create)(vfs_node_t *parent, const char *name, vfs_node_t **out);
} vfs_node_ops_t;

extern list_t g_vfs_all;

/**
 * @brief Mount a VFS on path
 * @param data private VFS data
 * @returns 0 on success, -errno on failure
 */
int vfs_mount(vfs_ops_t *vfs_ops, char *path, void *data);

/**
 * @brief Lookup a node by path
 * @returns 0 on success, -errno on failure
 */
int vfs_lookup(char *path, vfs_node_t **out, vfs_context_t *context);

/**
 * @brief Read/write a file by path
 * @param rw_count bytes read/written (out)
 * @returns 0 on success, -errno on failure
 */
int vfs_rw(char *path, vfs_rw_t *packet, size_t *rw_count, vfs_context_t *context);

/**
 * @brief Create a directory at path
 * @returns 0 on success, -errno on failure
 */
int vfs_mkdir(char *path, const char *name, vfs_node_t **out, vfs_context_t *context);

/**
 * @brief Create a file at path
 * @returns 0 on success, -errno on failure
 */
int vfs_create(char *path, const char *name, vfs_node_t **out, vfs_context_t *context);