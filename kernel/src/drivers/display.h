#ifndef DRIVERS_DISPLAY_H
#define DRIVERS_DISPLAY_H

#include <stdint.h>

typedef struct {
	uint16_t attributes;
	uint8_t window_a;
	uint8_t window_b;
	uint16_t granularity;
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;
	uint16_t pitch;
	uint16_t width;
	uint16_t height;
	uint8_t w_char;
	uint8_t y_char;
	uint8_t planes;
	uint8_t bpp;
	uint8_t banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;

	uint8_t direct_color_attributes;
	uint32_t framebuffer;
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;

    uint16_t bytes_per_scan_line;
    uint8_t number_of_images_banked;
    uint8_t number_of_images_linear;
    uint8_t linear_red_mask_size;
    uint8_t linear_red_mask_position;
    uint8_t linear_green_mask_size;
    uint8_t linear_green_mask_position;
    uint8_t linear_blue_mask_size;
    uint8_t linear_blue_mask_position;
    uint8_t linear_reserved_mask_size;
    uint8_t linear_reserved_mask_position;
    uint32_t maximum_pixel_clock;
    uint8_t reserved_2[190];
} __attribute__ ((packed)) vbe_mode_info_t;

void initialize_display(uint64_t vbe_mode_info_address);
vbe_mode_info_t *get_display_mode_info();

uint32_t get_pixel(int x, int y);
uint32_t create_color(uint8_t r, uint8_t g, uint8_t b);
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_char(int x, int y, char c, uint32_t fgcolor, uint32_t bgcolor);
void draw_copy(int src_x, int src_y, int dest_x, int dest_y, int w, int h);

#endif