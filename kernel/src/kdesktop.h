#ifndef KDESKTOP_H
#define KDESKTOP_H

#include <stdint.h>
#include <graphics/draw.h>

typedef struct kdesktop_window_struct {
    int x;
    int y;
    char *title;
    draw_context_t *ctx;
    struct kdesktop_window_struct *next;
} kdesktop_window_t;

void kdesktop_initialize(draw_context_t *ctx);
draw_context_t *kdesktop_create_window(int x, int y, uint16_t width, uint16_t height, char* title);

#endif