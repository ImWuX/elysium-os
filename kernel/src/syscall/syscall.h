#pragma once
#include <stdint.h>

typedef struct {
    uint64_t value;
    uint64_t errno;
} syscall_return_t;