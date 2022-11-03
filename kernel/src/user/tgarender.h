#ifndef USER_TGARENDER_H
#define USER_TGARENDER_H

#include <stdint.h>
#include <fs/fat32.h>

typedef struct {
    uint16_t first_entry_index;
    uint16_t entry_count;
    uint8_t bits_per_entry;
} __attribute__((packed)) tga_color_map_spec_t;

typedef struct {
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t image_descriptor;
} __attribute__((packed)) tga_image_spec_t;

typedef struct {
    uint8_t id_length;
    uint8_t color_map_type;
    uint8_t image_type;
    tga_color_map_spec_t color_map_spec;
    tga_image_spec_t image_spec;
} __attribute__((packed)) tga_header_t;

void draw_image(file_descriptor_t *imagefd, uint16_t x, uint16_t y);

#endif