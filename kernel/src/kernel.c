#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <panic.h>
#include <stdio.h>
#include <string.h>
#include <arch/types.h>
#include <arch/init.h>
#include <arch/vmm.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>

uintptr_t g_hhdm_address;

static draw_context_t g_ctx;
static int g_x = 0, g_y = 0;
int putchar(int c) {
    switch(c) {
        case '\n':
            g_x = 0;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(&g_ctx, g_x, g_y, (char) c, 0xFFFFFF);
            g_x += BASICFONT_WIDTH;
            break;
    }
    return (char) c;
}

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    if(boot_params->hhdm_start < ARCH_HHDM_START || boot_params->hhdm_end >= ARCH_HHDM_END) panic("KERNEL", "HHDM is not within arch specific boundaries");
    g_hhdm_address = boot_params->hhdm_start;
    uintptr_t hhdm_end = boot_params->hhdm_end;

    g_ctx.address = (void *) HHDM(boot_params->framebuffer->address);
    g_ctx.width = boot_params->framebuffer->width;
    g_ctx.height = boot_params->framebuffer->height;
    g_ctx.pitch = boot_params->framebuffer->pitch / (boot_params->framebuffer->bpp / 8);

    arch_init(boot_params);

    for(uint16_t i = 0; i < boot_params->memory_map_length; i++) {
        tartarus_memap_entry_t entry = boot_params->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base_address, entry.length);
    }

#ifdef __ARCH_AMD64
    uint64_t sp;
    asm volatile("mov %%rsp, %0" : "=rm" (sp));
    asm volatile("mov %0, %%rsp" : : "rm" (HHDM(sp)));
    uint64_t bp;
    asm volatile("mov %%rbp, %0" : "=rm" (bp));
    asm volatile("mov %0, %%rbp" : : "rm" (HHDM(bp)));
#endif

    vmm_address_space_t kernel_address_space;
    arch_vmm_create_kernel_address_space(&kernel_address_space);
    arch_vmm_load_address_space(&kernel_address_space);
    heap_initialize(&kernel_address_space, ARCH_KHEAP_START, ARCH_KHEAP_END);

    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n\n");
    printf("Welcome to Elysium OS\n");

    pmm_stats_t *stats = pmm_stats();
    printf("PMM Stats (in pages of %#lx):\n  >> Free: %lu\n  >> Wired: %lu\n  >> Anon: %lu\n  >> Backed: %lu\n", ARCH_PAGE_SIZE, stats->free_pages, stats->wired_pages, stats->anon_pages, stats->backed_pages);

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}