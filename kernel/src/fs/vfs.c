#include "vfs.h"

static vfs_block_t *g_root_block;

void vfs_initialize(vfs_block_t *root_block) {
    g_root_block = root_block;
}

// vfs_fd_t vfs_open(char *path) {
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