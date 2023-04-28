#ifndef GRAPHICS_DRAW_H
#define GRAPHICS_DRAW_H

#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    void *address;
    uint16_t pitch;
    bool invalidated;
} draw_context_t;

typedef uint32_t draw_color_t;

/**
 * @brief Packs RGB values into a color value
 * 
 * @param r Red comp
 * @param g Green comp
 * @param b Blue comp
 * @return The packed color 
 */
draw_color_t draw_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Retrieves a pixel from a context
 * 
 * @param ctx The draw context
 * @param x X Coord
 * @param y Y Coord
 * @return The pixel at the specified location
 */
draw_color_t draw_getpixel(draw_context_t *ctx, int x, int y);

/**
 * @brief Draws a character to a context
 * 
 * @param ctx The draw context
 * @param x X Coord
 * @param y Y Coord
 * @param c Character to draw
 * @param color Draw color
 */
void draw_char(draw_context_t *ctx, int x, int y, char c, draw_color_t color);

/**
 * @brief Draw a simple string to a context
 * 
 * @param ctx The draw context
 * @param x X Coord
 * @param y Y Coord
 * @param str String to draw
 * @param color Draw color
 */
void draw_string_simple(draw_context_t *ctx, int x, int y, char *str, draw_color_t color);

/**
 * @brief Draw a pixel to a context
 * 
 * @param ctx The draw context
 * @param x X Coord
 * @param y Y Coord
 * @param color Draw color
 */
void draw_pixel(draw_context_t *ctx, int x, int y, draw_color_t color);

/**
 * @brief Draw a rectangle to a context
 * 
 * @param ctx The draw context
 * @param x X Coord
 * @param y Y Coord
 * @param w Rectangle width
 * @param h Rectangle height
 * @param color Draw color
 */
void draw_rect(draw_context_t *ctx, int x, int y, uint16_t w, uint16_t h, draw_color_t color);

#endif