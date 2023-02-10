#ifndef KDESKTOP_H
#define KDESKTOP_H

#include <stdint.h>
#include <graphics/draw.h>

typedef struct kdesktop_window_struct {
    uint16_t x;
    uint16_t y;
    char *title;
    draw_context_t *ctx;
    struct kdesktop_window_struct *next;
} kdesktop_window_t;

void kdesktop_initialize(draw_context_t *ctx);

#endif