#ifndef FS_VFS_H
#define FS_VFS_H

#include <stdint.h>
#include <stdbool.h>

#define VFS_FD_STDIN 0
#define VFS_FD_STDOUT 1
#define VFS_FD_STDERR 2
#define VFS_FD_ERROR UINT64_MAX

// typedef uint64_t vfs_fd;

// vfs_fd vfs_open(char *path);
// bool vfs_close(vfs_fd fd);
// uint64_t vfs_write(vfs_fd fd, void *data, uint64_t size);
// uint64_t vfs_seekto(vfs_fd fd, uint64_t position);
// uint64_t vfs_seek(vfs_fd fd, uint64_t count);
// uint64_t vfs_read(vfs_fd fd, uint64_t count);

#endif