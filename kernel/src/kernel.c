#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <arch/arch_init.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>
#include <string.h>

uintptr_t g_hhdm_address;

static draw_context_t g_ctx;
static int g_x = 0, g_y = 0;
void putchar(char c) {
    switch(c) {
        case '\n':
            g_x = 0;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(&g_ctx, g_x, g_y, c, 0xFFFFFF);
            g_x += BASICFONT_WIDTH;
            break;
    }
}

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;

    g_ctx.address = HHDM(boot_params->framebuffer->address);
    g_ctx.width = boot_params->framebuffer->width;
    g_ctx.height = boot_params->framebuffer->height;
    g_ctx.pitch = boot_params->framebuffer->pitch / (boot_params->framebuffer->bpp / 8);

    for(uint16_t i = 0; i < boot_params->memory_map_length; i++) {
        tartarus_memap_entry_t entry = boot_params->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base_address, entry.length);
    }

    arch_init(boot_params);

    printf("Welcome to ElysiumOS\n");

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}