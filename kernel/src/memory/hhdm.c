#include "hhdm.h"
#include <memory/vmm.h>

uintptr_t g_hhdm_address;

inline void hhdm_map(void *address) {
    vmm_map(address, (void *) HHDM(address));
}