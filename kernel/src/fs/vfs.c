#include "vfs.h"

// vfs_fd vfs_open(char *path) {
//     int tail = 0;
//     int index = 0;
//     int last_invalid = -1;
//     bool is_absolute = false;
//     bool is_relative = false;
//     bool is_parent = false;
//     do {
//         switch(path[index]) {
//             case 0:
//             case '/':
//                 if(last_invalid > 0 && last_invalid == index - 1) return VFS_FD_ERROR;
//                 if(index == 0) is_absolute = true;
//                 if(is_relative) {
//                     is_relative = false;
//                     break;
//                 }
//                 if(is_parent) {
//                     printf("Parent Directory\n");
//                     is_parent = false;
//                     break;
//                 }
//                 if(tail != index) {
//                     for(int i = tail; i < index; i++) {
//                         printf("%c", path[i]);
//                     }
//                     printf("\n");
//                 }
//                 tail = index + 1;
//                 break;
//             case '.':
//                 if(tail == index) {
//                     is_relative = true;
//                     break;
//                 }
//                 if(is_relative) {
//                     is_relative = false;
//                     is_parent = true;
//                     break;
//                 }
//             case ' ':
//                 if(tail == index) return VFS_FD_ERROR;
//                 last_invalid = index;
//             default:
//                 if(is_parent || is_relative) return VFS_FD_ERROR;
//                 break;
//         }
//     } while(path[index++]);
//     return 3;
// }

// bool vfs_close(vfs_fd fd) {
//     return false;
// }

// uint64_t vfs_seekto(vfs_fd fd, uint64_t position) {
//     return 0;
// }

// uint64_t vfs_seek(vfs_fd fd, uint64_t count) {
//     return 0;
// }

// uint64_t vfs_write(vfs_fd fd, void *data, uint64_t size) {
//     switch(fd) {
//         case VFS_FD_STDIN:
//             return 0;
//         case VFS_FD_STDOUT:
//         case VFS_FD_STDERR:
            
//             return size;
//         default:

//             break;
//     }
//     return 0;
// }

// uint64_t vfs_read(vfs_fd fd, uint64_t count) {
//     switch(fd) {
//         case VFS_FD_STDIN:
//             return count;
//         case VFS_FD_STDOUT:
//         case VFS_FD_STDERR:
//             return 0;
//         default:

//             break;
//     }
//      return 0;
// }