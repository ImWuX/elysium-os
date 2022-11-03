#include "vfs.h"
#include <drivers/display.h>

uint64_t vfs_write(uint64_t fd, void *data, uint64_t size) {
    switch(fd) {
        case VFS_FD_STDIN:
            return 0;
        case VFS_FD_STDOUT:
        case VFS_FD_STDERR:

            return size;
        default:

            break;
    }
}
