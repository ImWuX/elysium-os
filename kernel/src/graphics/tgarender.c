#include "tgarender.h"
#include <stddef.h>
#include <lib/kprint.h>
#include <memory/heap.h>

#define IMG_TYPE_RLE (1 << 4)
#define IMG_DESC_ORDER_RIGHT_TO_LEFT (1 << 4)
#define IMG_DESC_ORDER_TOP_TO_BOTTOM (1 << 5)

typedef struct {
    uint16_t first_entry_index;
    uint16_t entry_count;
    uint8_t bits_per_entry;
} __attribute__((packed)) color_map_specification_t;

typedef struct {
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t image_descriptor;
} __attribute__((packed)) image_specification_t;

typedef struct {
    uint8_t id_length;
    uint8_t color_map_type;
    uint8_t image_type;
    color_map_specification_t color_map;
    image_specification_t image;
} __attribute__((packed)) tga_header_t;

void tgarender_render(draw_context_t *ctx, void *image, int origin_x, int origin_y) {
    tga_header_t *header = image;
    if(header->color_map_type != 0) {
        kprintf("TGA: Color maps are unsupported. Cancelling render.\n");
        return;
    }

    if(header->image_type != 2) {
        kprintf("TGA: Only uncompressed true-color images are supported. Cancelling render.\n");
        return;
    }

    if(header->image_type & IMG_TYPE_RLE) {
        kprintf("TGA: RLE compression not supported. Cancelling render.\n");
        return;
    }

    if(header->image.bpp != 32) {
        kprintf("TGA: Only the 32bpp pixel format is supported. Cancelling render.\n");
        return;
    }

    uintptr_t offset = header->id_length + (header->color_map.entry_count * header->color_map.bits_per_entry + 7) / 8 + sizeof(tga_header_t);
    for(int x = 0; x < header->image.width; x++) {
        for(int y = 0; y < header->image.height; y++) {
            uint8_t *pixel = (uint8_t *) ((uintptr_t) image + offset + (y * header->image.width + x) * (header->image.bpp / 8));
            int adjusted_x = x;
            if(header->image.image_descriptor & IMG_DESC_ORDER_RIGHT_TO_LEFT) adjusted_x = header->image.width - x;
            int adjusted_y = header->image.height - y;
            if(header->image.image_descriptor & IMG_DESC_ORDER_TOP_TO_BOTTOM) adjusted_y = y;
            draw_pixel(ctx, origin_x + adjusted_x, origin_y + adjusted_y, draw_color(pixel[2], pixel[1], pixel[0]));
        }
    }
}