#ifndef GRAPHICS_DRAW_H
#define GRAPHICS_DRAW_H

#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint16_t pitch;
    uintptr_t address;
} draw_framebuffer_t;

typedef struct {
    uint8_t red_size;
    uint8_t red_shift;
    uint8_t green_size;
    uint8_t green_shift;
    uint8_t blue_size;
    uint8_t blue_shift;
} draw_colormask_t;

typedef uint32_t draw_color_t;

void draw_initialize(draw_colormask_t mask, draw_framebuffer_t buffer);
draw_color_t draw_create_color(uint8_t r, uint8_t g, uint8_t b);

void draw_char(uint16_t x, uint16_t y, char c, draw_color_t fgcolor, draw_color_t bgcolor);
void draw_pixel(uint16_t x, uint16_t y, draw_color_t color);
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, draw_color_t color);

#endif