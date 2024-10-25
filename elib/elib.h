#pragma once
#include <stdint.h>

typedef struct {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} elib_framebuffer_info_t;

void *elib_acquire_framebuffer(elib_framebuffer_info_t *info);
int elib_input();