#include <tartarus.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <lib/kprint.h>
#include <lib/panic.h>
#include <lib/string.h>
#include <arch/cpu.h>
#include <arch/types.h>
#include <arch/x86_64/port.h>

uintptr_t g_hhdm_base;

static volatile int g_cpus_initialized;

static void pch(char ch) {
    port_outb(0x3F8, ch);
}

static void init_common() {
    kprintf("CPU %i\n", g_cpus_initialized);

    __atomic_add_fetch(&g_cpus_initialized, 1, __ATOMIC_SEQ_CST);
}

[[noreturn]] __attribute__((naked)) void init_ap() {
    init_common();

    arch_cpu_halt();
    __builtin_unreachable();
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    g_hhdm_base = boot_info->hhdm_base;

    g_kprint_putchar = pch;

    kprintf("Elysium pre-alpha\n");

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(memcmp(module->name, "KERNSYMBTXT", 11) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    pmm_zone_create(PMM_ZONE_INDEX_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_create(PMM_ZONE_INDEX_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        if(entry.base < 0x100'0000) {
            if(entry.base + entry.length > 0x100'0000) {
                pmm_region_add(PMM_ZONE_INDEX_DMA, entry.base, 0x100'0000 - entry.base);
                pmm_region_add(PMM_ZONE_INDEX_NORMAL, 0x100'0000, entry.length - (0x100'0000 - entry.base));
            } else {
                pmm_region_add(PMM_ZONE_INDEX_DMA, entry.base, entry.length);
            }
        } else {
            pmm_region_add(PMM_ZONE_INDEX_NORMAL, entry.base, entry.length);
        }
    }

    g_cpus_initialized = 0;
    for(int i = 0; i < boot_info->cpu_count; i++) {
        if(i == boot_info->bsp_index) {
            init_common();
            continue;
        }
        *boot_info->cpus[i].wake_on_write = (uint64_t) &init_ap;
        while(i >= g_cpus_initialized);
    }

    arch_cpu_halt();
    __builtin_unreachable();
}