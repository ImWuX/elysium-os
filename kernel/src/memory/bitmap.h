#ifndef MEMORY_BITMAP_H
#define MEMORY_BITMAP_H

#include <stdint.h>
#include <stdbool.h>

#define BITMAP_BLOCK_SIZE uint64_t
#define BITMAP_BLOCK_FILL UINT64_MAX

void bitmap_initialize(uintptr_t address, uint64_t size);
void *bitmap_request();
void bitmap_release(void *address);

#endif