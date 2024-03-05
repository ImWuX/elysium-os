#pragma once
#include <stdint.h>
#include <graphics/font.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    void *address;
    uint16_t pitch;
} draw_context_t;

typedef uint32_t draw_color_t;

/**
 * @brief Packs RGB values into a color value
 */
draw_color_t draw_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Retrieves a pixel from a context
 * @returns pixel at the specified coordinate
 */
draw_color_t draw_getpixel(draw_context_t *ctx, int x, int y);

/**
 * @brief Draws a character to a context
 */
void draw_char(draw_context_t *ctx, int x, int y, char c, font_t *font, draw_color_t color);

/**
 * @brief Draw a simple string to a context
 */
void draw_string_simple(draw_context_t *ctx, int x, int y, char *str, font_t *font, draw_color_t color);

/**
 * @brief Draw a pixel to a context
 */
void draw_pixel(draw_context_t *ctx, int x, int y, draw_color_t color);

/**
 * @brief Draw a rectangle to a context
 * @param w rectangle width
 * @param h rectangle height
 */
void draw_rect(draw_context_t *ctx, int x, int y, uint16_t w, uint16_t h, draw_color_t color);