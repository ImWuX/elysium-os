#include "draw.h"
#include <memory/hhdm.h>
#include <graphics/basicfont.h>

static draw_colormask_t g_mask;
static draw_framebuffer_t g_buffer;
static uint16_t g_pixel_width;
static uint16_t g_pixel_pitch;

void draw_initialize(draw_colormask_t mask, draw_framebuffer_t buffer) {
    g_mask = mask;
    g_buffer = buffer;
    g_pixel_width = (g_buffer.bpp + 7) / 8;
    g_pixel_pitch = g_buffer.pitch / g_pixel_width;
}

draw_color_t draw_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r & ((1 << g_mask.red_size) - 1)) << g_mask.red_shift |
        (g & ((1 << g_mask.green_size) - 1)) << g_mask.green_shift |
        (b & ((1 << g_mask.blue_size) - 1)) << g_mask.blue_shift;
}

uint16_t draw_scrw() {
    return g_buffer.width;
}

uint16_t draw_scrh() {
    return g_buffer.height;
}

void draw_char(uint16_t x, uint16_t y, char c, draw_color_t fgcolor, draw_color_t bgcolor) {
    uint8_t *font_char = &g_basicfont[c * 16];

    uint32_t *buf = (uint32_t *) HHDM(g_buffer.address);
    uint64_t offset = x + y * g_pixel_pitch;
    for(uint16_t yy = 0; yy < BASICFONT_HEIGHT; yy++) {
        for(uint16_t xx = 0; xx < BASICFONT_WIDTH; xx++) {
            if(font_char[yy] & (1 << (BASICFONT_WIDTH - xx))){
                buf[offset + xx] = fgcolor;
            } else {
                buf[offset + xx] = bgcolor;
            }
        }
        offset += g_pixel_pitch;
    }
}

void draw_string_simple(uint16_t x, uint16_t y, char *str, draw_color_t fgcolor, draw_color_t bgcolor) {
    while(*str) {
        draw_char(x, y, *str, fgcolor, bgcolor);
        x += BASICFONT_WIDTH;
        str++;
    }
}

void draw_pixel(uint16_t x, uint16_t y, draw_color_t color) {
    *(uint32_t *) (HHDM(g_buffer.address) + y * g_buffer.pitch + x * g_pixel_width) = color;
}

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, draw_color_t color) {
    uint32_t *buf = (uint32_t *) HHDM(g_buffer.address);
    uint64_t offset = x + y * g_pixel_pitch;
    for(uint16_t yy = 0; yy < h; yy++) {
        for(uint16_t xx = 0; xx < w; xx++) {
            buf[offset + xx] = color;
        }
        offset += g_pixel_pitch;
    }
}