#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <stdio.h>
#include <arch/types.h>
#include <arch/arch_init.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
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
    g_hhdm_address = boot_params->hhdm_address;

    g_ctx.address = (void *) HHDM(boot_params->framebuffer->address);
    g_ctx.width = boot_params->framebuffer->width;
    g_ctx.height = boot_params->framebuffer->height;
    g_ctx.pitch = boot_params->framebuffer->pitch / (boot_params->framebuffer->bpp / 8);

    for(uint16_t i = 0; i < boot_params->memory_map_length; i++) {
        tartarus_memap_entry_t entry = boot_params->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base_address, entry.length);
    }

    arch_init(boot_params);

    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n\n");
    printf("Welcome to Elysium OS\n");

    pmm_stats_t *stats = pmm_stats();
    printf("PMM Stats (in pages of %#lx):\n -> Free: %lu\n -> Wired: %lu\n -> Anon: %lu\n -> Backed: %lu\n", PAGE_SIZE, stats->free_pages, stats->wired_pages, stats->anon_pages, stats->backed_pages);

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}