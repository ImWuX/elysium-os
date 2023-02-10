#include "draw.h"
#include <graphics/basicfont.h>

inline static void putpixel(draw_context_t *ctx, uint64_t offset, draw_color_t color) {
    if(offset > ctx->height * ctx->pitch * sizeof(draw_color_t)) return;
    ((draw_color_t *) ctx->address)[offset] = color;
}

inline static draw_color_t getpixel(draw_context_t *ctx, uint64_t offset) {
    if(offset > ctx->height * ctx->pitch * sizeof(draw_color_t)) return 0;
    return ((draw_color_t *) ctx->address)[offset];
}

draw_color_t draw_color(draw_context_t *ctx, uint8_t r, uint8_t g, uint8_t b) {
    return
        ((r & ((1 << ctx->colormask->red_size) - 1)) << ctx->colormask->red_shift) |
        ((g & ((1 << ctx->colormask->green_size) - 1)) << ctx->colormask->green_shift) |
        ((b & ((1 << ctx->colormask->blue_size) - 1)) << ctx->colormask->blue_shift);
}

draw_color_t draw_getpixel(draw_context_t *ctx, uint16_t x, uint16_t y) {
    return getpixel(ctx, y * ctx->pitch + x);
}

void draw_char(draw_context_t *ctx, uint16_t x, uint16_t y, char c, draw_color_t color) {
    uint8_t *font_char = &g_basicfont[c * 16];

    uint64_t offset = x + y * ctx->pitch;
    for(uint16_t yy = 0; yy < BASICFONT_HEIGHT; yy++) {
        for(uint16_t xx = 0; xx < BASICFONT_WIDTH; xx++) {
            if(font_char[yy] & (1 << (BASICFONT_WIDTH - xx))) {
                putpixel(ctx, offset + xx, color);
            }
        }
        offset += ctx->pitch;
    }
}

void draw_string_simple(draw_context_t *ctx, uint16_t x, uint16_t y, char *str, draw_color_t color) {
    while(*str) {
        draw_char(ctx, x, y, *str++, color);
        x += BASICFONT_WIDTH;
    }
}

void draw_pixel(draw_context_t *ctx, uint16_t x, uint16_t y, draw_color_t color) {
    putpixel(ctx, y * ctx->pitch + x, color);
}

void draw_rect(draw_context_t *ctx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, draw_color_t color) {
    uint64_t offset = x + y * ctx->pitch;
    for(uint16_t yy = 0; yy < h; yy++) {
        for(uint16_t xx = 0; xx < w; xx++) {
            putpixel(ctx, offset + xx, color);
        }
        offset += ctx->pitch;
    }
}