#ifndef HAL_VFS_H
#define HAL_VFS_H

#include <stdint.h>
#include <stdbool.h>

#define VFS_FD_STDIN 0
#define VFS_FD_STDOUT 1
#define VFS_FD_STDERR 2

uint64_t vfs_write(uint64_t fd, void *data, uint64_t size);

#endif