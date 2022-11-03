#include "display.h"
#include <memory/vmm.h>
#include <util/vesa_font.h>

static vbe_mode_info_t *g_mode_info;
static uint8_t pixel_width;
static uint16_t pixels_per_row;

void initialize_display(uint64_t vbe_mode_info_address) {
    g_mode_info = (vbe_mode_info_t *) vbe_mode_info_address;

    for(int i = 0; i < g_mode_info->bytes_per_scan_line * g_mode_info->height / 0x1000; i++) {
        uint64_t address = ((uint64_t) g_mode_info->framebuffer) + i * 0x1000;
        map_memory((void *) address, (void *) address);
    }
    pixel_width = (g_mode_info->bpp + 7) / 8;
    pixels_per_row = g_mode_info->bytes_per_scan_line / pixel_width;
}

vbe_mode_info_t *get_display_mode_info() {
    return g_mode_info;
}

uint32_t create_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << g_mode_info->linear_red_mask_position) + (g << g_mode_info->linear_green_mask_position) + (b << g_mode_info->linear_blue_mask_position);
}

uint32_t get_pixel(int x, int y) {
    return *(uint32_t *) (uint64_t) (y * g_mode_info->bytes_per_scan_line + x * pixel_width + g_mode_info->framebuffer);
}

void draw_pixel(int x, int y, uint32_t color) {
    *(uint32_t *) (uint64_t) (y * g_mode_info->bytes_per_scan_line + x * pixel_width + g_mode_info->framebuffer) = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    uint32_t *buf = (uint32_t *) (uint64_t) g_mode_info->framebuffer;
    int offset = x + y * pixels_per_row;
    for(int yy = 0; yy < h; yy++) {
        for(int xx = 0; xx < w; xx++) {
            buf[offset + xx] = color;
        }
        offset += pixels_per_row;
    }
}

void draw_char(int x, int y, char c, uint32_t fgcolor, uint32_t bgcolor) {
    uint8_t *font_char = &g_vesa_font[c * 16];

    uint32_t *buf = (uint32_t *) (uint64_t) g_mode_info->framebuffer;
    int offset = x + y * pixels_per_row;
    for(int xx = 0; xx < FONT_HEIGHT; xx++) {
        for(int yy = 0; yy < FONT_WIDTH; yy++) {
            if(font_char[xx] & (1 << (FONT_WIDTH - yy))){
                buf[offset + yy] = fgcolor;
            } else {
                buf[offset + yy] = bgcolor;
            }
        }
        offset += pixels_per_row;
    }
}

void draw_copy(int src_x, int src_y, int dest_x, int dest_y, int w, int h) {
    uint32_t *buf = (uint32_t *) (uint64_t) g_mode_info->framebuffer;
    int src_offset = src_x + src_y * pixels_per_row;
    int dest_offset = dest_x + dest_y * pixels_per_row;
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            buf[dest_offset + x] = buf[src_offset + x];
        }
        src_offset += pixels_per_row;
        dest_offset += pixels_per_row;
    }
}
