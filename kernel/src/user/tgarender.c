#include "tgarender.h"
#include <stdio.h>
#include <drivers/display.h>
#include <memory/heap.h>

void draw_image(file_descriptor_t *imagefd, uint16_t posx, uint16_t posy) {
    printf("Drawing image ");
    for(int i = 0; i < 11; i++) {
        putchar(imagefd->name[i]);
    }
    printf("\n");

    void *imagebuf = malloc(imagefd->file_size);
    fread(imagefd, imagefd->file_size, imagebuf);

    tga_header_t *header = imagebuf;
    printf("Type: [%i], Color Map Type: [%i], Size: [%iw %ih]\n", header->image_type, header->color_map_type, header->image_spec.width, header->image_spec.height);
    printf("BPP: [%i], Origin: (%i, %i), Image Descriptor: %x\n", header->image_spec.bpp / 8, header->image_spec.x_origin, header->image_spec.y_origin, header->image_spec.image_descriptor);

    if(header->color_map_type != 0) {
        printe("Unsupported color map type");
    } else {
        void *image_data = (uint8_t *) imagebuf + header->id_length + header->color_map_spec.entry_count * header->color_map_spec.bits_per_entry;
        for(int x = 0; x < header->image_spec.width; x++) {
            for(int y = 0; y < header->image_spec.height; y++) {
                uint8_t *pixel = image_data + (y * header->image_spec.width + x) * (header->image_spec.bpp / 8);
                draw_pixel(posx + x, posy + y, create_color(pixel[0], pixel[1], pixel[2]));
            }
        }
    }
    free(imagebuf);
}