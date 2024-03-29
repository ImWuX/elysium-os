#include "draw.h"
#include <graphics/font.h>

static inline void putpixel(draw_context_t *ctx, uint64_t offset, draw_color_t color) {
    if(offset > ctx->height * ctx->pitch * sizeof(draw_color_t)) return;
    ((draw_color_t *) ctx->address)[offset] = color;
}

static inline draw_color_t getpixel(draw_context_t *ctx, uint64_t offset) {
    if(offset > ctx->height * ctx->pitch * sizeof(draw_color_t)) return 0;
    return ((draw_color_t *) ctx->address)[offset];
}

draw_color_t draw_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | (b << 0);
}

draw_color_t draw_getpixel(draw_context_t *ctx, int x, int y) {
    return getpixel(ctx, y * ctx->pitch + x);
}

void draw_char(draw_context_t *ctx, int x, int y, char c, font_t *font, draw_color_t color) {
    int w = font->width;
    int h = font->height;
    if(x < 0) {
        if(w <= -x) return;
        w += x;
        x = 0;
    }
    if(y < 0) {
        if(h <= -y) return;
        h += y;
        y = 0;
    }
    uint8_t *font_char = &font->data[((unsigned int) (uint8_t) c) * font->width * font->height / 8];

    uint64_t offset = x + y * ctx->pitch;
    for(int yy = 0; yy < h && y + yy < ctx->height; yy++) {
        for(int xx = 0; xx < w && x + xx < ctx->width; xx++) {
            if(font_char[yy] & (1 << (w - xx))) {
                putpixel(ctx, offset + xx, color);
            }
        }
        offset += ctx->pitch;
    }
}

void draw_string_simple(draw_context_t *ctx, int x, int y, char *str, font_t *font, draw_color_t color) {
    while(*str) {
        draw_char(ctx, x, y, *str++, font, color);
        x += font->width;
    }
}

void draw_pixel(draw_context_t *ctx, int x, int y, draw_color_t color) {
    if(x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return;
    putpixel(ctx, y * ctx->pitch + x, color);
}

void draw_rect(draw_context_t *ctx, int x, int y, uint16_t w, uint16_t h, draw_color_t color) {
    if(x < 0) {
        if(w <= -x) return;
        w += x;
        x = 0;
    }
    if(y < 0) {
        if(h <= -y) return;
        h += y;
        y = 0;
    }
    uint64_t offset = x + y * ctx->pitch;
    for(int yy = 0; yy < h && y + yy < ctx->height; yy++) {
        for(int xx = 0; xx < w && x + xx < ctx->width; xx++) {
            putpixel(ctx, offset + xx, color);
        }
        offset += ctx->pitch;
    }
}