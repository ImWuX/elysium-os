#ifndef GRAPHICS_DRAW_H
#define GRAPHICS_DRAW_H

#include <stdint.h>
#include <stdbool.h>
#include <graphics/draw.h>

typedef struct {
    uint8_t red_size;
    uint8_t red_shift;
    uint8_t green_size;
    uint8_t green_shift;
    uint8_t blue_size;
    uint8_t blue_shift;
} draw_colormask_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    void *address;
    uint16_t pitch;
    draw_colormask_t *colormask;
    bool invalidated;
} draw_context_t;

typedef uint32_t draw_color_t;

draw_color_t draw_color(draw_context_t *ctx, uint8_t r, uint8_t g, uint8_t b);

draw_color_t draw_getpixel(draw_context_t *ctx, uint16_t x, uint16_t y);

void draw_char(draw_context_t *ctx, uint16_t x, uint16_t y, char c, draw_color_t color);
void draw_string_simple(draw_context_t *ctx, uint16_t x, uint16_t y, char *str, draw_color_t color);
void draw_pixel(draw_context_t *ctx, uint16_t x, uint16_t y, draw_color_t color);
void draw_rect(draw_context_t *ctx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, draw_color_t color);

#endif