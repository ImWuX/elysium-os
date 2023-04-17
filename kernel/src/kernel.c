#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>

uintptr_t g_hhdm_address;

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;

    for(uint16_t i = 0; i < boot_params->memory_map_length; i++) {
        tartarus_memap_entry_t entry = boot_params->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base_address, entry.length);
    }

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}