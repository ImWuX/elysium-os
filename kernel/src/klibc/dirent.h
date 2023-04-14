#ifndef KLIBC_DIRENT_H
#define KLIBC_DIRENT_H

#include <sys/types.h>
#include <limits.h>

struct dirent {
    ino_t d_ino;
    char d_name[NAME_MAX + 1];
};

typedef struct dirent dirent_t;

#endif