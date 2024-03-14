#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uintptr_t phys_address;
    size_t size;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
} framebuffer_t;

extern framebuffer_t g_framebuffer;