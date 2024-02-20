#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <tartarus.h>
#include <lib/format.h>
#include <common/kprint.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <arch/cpu.h>
#include <arch/x86_64/port.h>

uintptr_t g_hhdm_base;
size_t g_hhdm_size;

static void pch(char ch) {
	x86_64_port_outb(0x3F8, ch);
}

int kprintv(const char *fmt, va_list list) {
	return format(pch, fmt, list);
}

int kprintf(const char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
	int ret = kprintv(fmt, list);
	va_end(list);
	return ret;
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
	g_hhdm_base = boot_info->hhdm_base;
	g_hhdm_size = boot_info->hhdm_size;

	kprintf("Elysium Alpha\n");
    kprintf("HHDM: %#lx (%#lx)\n", g_hhdm_base, g_hhdm_size);

	// Initialize physical memory
    pmm_zone_register(PMM_ZONE_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_register(PMM_ZONE_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base, entry.length);
    }

    kprintf("Physical Memory Map\n");
    for(int i = 0; i <= PMM_ZONE_MAX; i++) {
        pmm_zone_t *zone = &g_pmm_zones[i];
        if(!zone->present) continue;

        kprintf("- %s\n", zone->name);
        LIST_FOREACH(&zone->regions, elem) {
            pmm_region_t *region = LIST_CONTAINER_GET(elem, pmm_region_t, list_elem);
            kprintf("  - %#-12lx %lu/%lu pages\n", region->base, region->free_count, region->page_count);
        }
    }


    arch_cpu_halt();
}