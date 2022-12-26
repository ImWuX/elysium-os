#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <stdio.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <graphics/basicfont.h>

uint64_t g_hhdm_address;

static tartarus_framebuffer_t g_framebuffer;
static uint16_t g_x;
static uint16_t g_y;

int putchar(int c) {
    switch(c) {
        case '\n':
            g_x = 0;
            g_y += BASIC_FONT_HEIGHT;
            break;
        default:
            uint8_t *font_char = &g_basic_font[c * 16];

            uint32_t *buf = (uint32_t *) (uint64_t) g_framebuffer.address;
            int offset = g_x + g_y * g_framebuffer.pitch / 4;
            for(int xx = 0; xx < BASIC_FONT_HEIGHT; xx++) {
                for(int yy = 0; yy < BASIC_FONT_WIDTH; yy++) {
                    if(font_char[xx] & (1 << (BASIC_FONT_WIDTH - yy))){
                        buf[offset + yy] = 0xFFFFFFFF;
                    } else {
                        buf[offset + yy] = 0;
                    }
                }
                offset += g_framebuffer.pitch / 4;
            }
            g_x += BASIC_FONT_WIDTH;
            break;
    }
    if(g_x >= g_framebuffer.width) {
        g_x = 0;
        g_y += BASIC_FONT_HEIGHT;
    }
    if(g_y >= g_framebuffer.height) g_y = g_framebuffer.height - BASIC_FONT_HEIGHT - 1;
    return c;
}

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;
    g_framebuffer = *boot_params->framebuffer;

    printf("Welcome to Elysium OS\n");

    pmm_initialize(boot_params->memory_map, boot_params->memory_map_length);

    printf("Physical Memory Initialized\n");

    uint64_t pml4;
    asm volatile("mov %%cr3, %0" : "=r" (pml4));
    vmm_initialize(pml4);

    while(true) asm("hlt");
}