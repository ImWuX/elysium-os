#ifndef MEMORY_FREELIST_H
#define MEMORY_FREELIST_H

#include <stdint.h>
#include <stdbool.h>

void *freelist_page_request();
void freelist_page_release(void *address);

#endif