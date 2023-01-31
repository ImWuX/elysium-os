#include "freelist.h"
#include <memory/hhdm.h>

#define PAGE_SIZE 0x1000

static uintptr_t g_freelist = 0;

bool freelist_add_region(uint64_t base, uint64_t length) {
    if(base % PAGE_SIZE != 0 || length % PAGE_SIZE != 0) return true;
    for(uint64_t address = base; address < base + length; address += PAGE_SIZE) {
        *((uintptr_t *) HHDM(address)) = g_freelist;
        g_freelist = address;
    }
    return false;
}

void *freelist_page_request() {
    if(!g_freelist) return 0;
    uintptr_t address = g_freelist;
    g_freelist = *((uintptr_t *) HHDM(address));
    return (void *) address;
}

void freelist_page_release(void *address) {
    *((uintptr_t *) HHDM(address)) = g_freelist;
    g_freelist = (uintptr_t) address;
}