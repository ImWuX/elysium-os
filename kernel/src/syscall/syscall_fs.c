#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <lib/str.h>
#include <lib/mem.h>
#include <common/log.h>
#include <syscall/syscall.h>
#include <memory/heap.h>
#include <arch/types.h>
#include <arch/sched.h>

syscall_return_t syscall_fs_open(int dir_resource_id, const char *path, int flags, mode_t mode) {
    syscall_return_t ret = {};

    // TODO: Should standardize copying to/from userspace
    char *safe_path = heap_alloc(PATH_MAX + 1);
    strncpy(safe_path, path, PATH_MAX);
    safe_path[PATH_MAX] = 0;

    log(LOG_LEVEL_DEBUG, "SYSCALL", "open(dir_resource_id: %i, path: %s, flags: %#i, mode: %u)", dir_resource_id, safe_path, flags, mode);

    process_t *proc = arch_sched_thread_current()->proc;

    vfs_node_t *cwd = NULL;
    if(dir_resource_id == AT_FDCWD) {
        cwd = proc->cwd;
    } else {
        resource_t *parent = resource_get(&proc->resource_table, dir_resource_id);
        if(parent == NULL) {
            ret.errno = EBADF;
            return ret;
        }
        cwd = parent->node;
    }

    vfs_node_t *node;
    int r = vfs_lookup(safe_path, &node, cwd);
    if(r != 0) {
        ret.errno = -r;
        return ret;
    }

    ret.value = (size_t) resource_create(&proc->resource_table, node);
    return ret;
}

syscall_return_t syscall_fs_close(int resource_id) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "close(resource_id: %i)", resource_id);

    process_t *proc = arch_sched_thread_current()->proc;
    int r = resource_remove(&proc->resource_table, resource_id);
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t syscall_fs_read(int resource_id, void *buf, size_t count) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "read(resource_id: %i, buf: %#lx, count: %#lx)", resource_id, (uint64_t) buf, count);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    // TODO: Should standardize copying to/from userspace
    void *safe_buf = heap_alloc(count);
    size_t read_count = 0;
    int r = resource->node->ops->rw(resource->node, &(vfs_rw_t) {
        .rw = VFS_RW_READ,
        .buffer = safe_buf,
        .size = count,
        .offset = resource->offset
    }, &read_count);
    memcpy(buf, safe_buf, read_count);
    heap_free(safe_buf);
    resource->offset += read_count;
    ret.value = read_count;
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t syscall_fs_write(int resource_id, void *buf, size_t count) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "write(resource_id: %i, buf: %#lx, count: %#lx)", resource_id, (uint64_t) buf, count);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    // TODO: Should standardize copying to/from userspace
    void *safe_buf = heap_alloc(count);
    memcpy(safe_buf, buf, count);

    size_t write_count = 0;
    int r = resource->node->ops->rw(resource->node, &(vfs_rw_t) {
        .rw = VFS_RW_WRITE,
        .buffer = safe_buf,
        .size = count,
        .offset = resource->offset
    }, &write_count);
    heap_free(safe_buf);
    resource->offset += write_count;
    ret.value = write_count;
    if(r != 0) ret.errno = -r;
    return ret;
}

syscall_return_t syscall_fs_seek(int resource_id, off_t offset, int whence) {
    syscall_return_t ret = {};
    log(LOG_LEVEL_DEBUG, "SYSCALL", "seek(resource_id: %i, offset: %#lx, whence: %i)", resource_id, offset, whence);

    process_t *proc = arch_sched_thread_current()->proc;
    resource_t *resource = resource_get(&proc->resource_table, resource_id);
    if(resource == NULL) {
        ret.errno = EBADF;
        return ret;
    }

    ssize_t current_offset = (ssize_t) resource->offset;
    ssize_t new_offset = 0;
    switch(whence) {
        case SEEK_CUR:
            new_offset = current_offset + offset;
            break;
        case SEEK_END:
            vfs_node_attr_t attr;
            int r = resource->node->ops->attr(resource->node, &attr);
            if(r != 0) {
                ret.errno = -r;
                return ret;
            }
            new_offset = offset + attr.file_size;
            break;
        case SEEK_SET:
            new_offset = offset;
            break;
        default:
            ret.errno = EINVAL;
            return ret;
    }

    if(new_offset < 0) {
        ret.errno = EINVAL;
        return ret;
    }

    // TODO: file should grow here already and not on write?

    resource->offset = (size_t) new_offset;
    ret.value = (size_t) new_offset;
    return ret;
}

syscall_return_t syscall_fs_attr(int resource_id, const char *path, int flags, struct stat *statbuf) {
    syscall_return_t ret = {};

    // TODO: Should standardize copying to/from userspace
    char *safe_path = heap_alloc(PATH_MAX + 1);
    strncpy(safe_path, path, PATH_MAX);
    safe_path[PATH_MAX] = 0;

    log(LOG_LEVEL_DEBUG, "SYSCALL", "attr(resource_id: %i, path: %s, flags: %i, statbuf: %#lx)", resource_id, safe_path, flags, (uintptr_t) statbuf);

    if(statbuf == NULL) {
        heap_free(safe_path);
        ret.errno = EINVAL;
        return ret;
    }

    process_t *proc = arch_sched_thread_current()->proc;

    vfs_node_t *parent;
    if(resource_id == AT_FDCWD) {
        parent = proc->cwd;
    } else {
        resource_t *resource = resource_get(&proc->resource_table, resource_id);
        if(resource == NULL) {
            heap_free(safe_path);
            ret.errno = EBADF;
            return ret;
        }
        parent = resource->node;
    }

    vfs_node_t *node;
    if(strlen(safe_path) == 0) {
        if((flags & AT_EMPTY_PATH) == 0) {
            heap_free(safe_path);
            ret.errno = ENOENT;
            return ret;
        }
        node = parent;
    } else {
        // TODO: (flags & AT_SYMLINK_NOFOLLOW) should be passed here
        int r = vfs_lookup(safe_path, &node, parent);
        if(r != 0) {
            ret.errno = -r;
            return ret;
        }
    }

    vfs_node_attr_t attr;
    int r = node->ops->attr(node, &attr);
    if(r != 0) {
        heap_free(safe_path);
        ret.errno = -r;
        return ret;
    }

    struct stat *safe_stat = heap_alloc(sizeof(struct stat));
    safe_stat->st_dev = 0; // TODO: set st_dev
	safe_stat->st_ino = 0; // TODO: set st_ino
	safe_stat->st_mode = 0; // TODO: set st_mode
	safe_stat->st_nlink = 0; // TODO: set st_nlink
	safe_stat->st_uid = 0; // TODO: set st_uid
	safe_stat->st_gid = 0; // TODO: set st_gid
	safe_stat->st_rdev = 0; // TODO: set st_rdev
	safe_stat->st_size = attr.file_size;
	safe_stat->st_blksize = 0; // TODO: set st_blksize
	safe_stat->st_blocks = 0; // TODO: set st_blocks
	safe_stat->st_atim.tv_sec = 0; // TODO: set st_atim
	safe_stat->st_atim.tv_nsec = 0;
	safe_stat->st_mtim.tv_sec = 0; // TODO: set st_mtim
	safe_stat->st_mtim.tv_nsec = 0;
	safe_stat->st_ctim.tv_sec = 0; // TODO: set st_ctim
	safe_stat->st_ctim.tv_nsec = 0;

    // TODO: Should standardize copying to/from userspace
    memcpy(statbuf, safe_stat, sizeof(struct stat));
    heap_free(safe_stat);
    return ret;
}