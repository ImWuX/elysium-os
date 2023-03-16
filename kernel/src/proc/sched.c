#include "sched.h"
#include <stdbool.h>
#include <memory/pmm.h>
#include <memory/vmm.h>

noreturn void sched_handoff() {
    vmm_mapf(pmm_page_request(), (void *) 0x1000, VMM_FLAG_READWRITE | VMM_FLAG_USER);
    uint8_t *spin_proc = (uint8_t *) 0x1000;
    spin_proc[0] = 0xEB;
    spin_proc[1] = 0xFE;
    sched_enter((uint64_t) spin_proc);
    __builtin_unreachable();
}