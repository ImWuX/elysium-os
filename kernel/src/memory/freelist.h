#ifndef MEMORY_FREELIST_H
#define MEMORY_FREELIST_H

#include <stdint.h>
#include <stdbool.h>

bool freelist_add_region(uint64_t base, uint64_t length);
void *freelist_page_request();
void freelist_page_release(void *address);

#endif