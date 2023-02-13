#include "kdesktop.h"
#include <stdbool.h>
#include <string.h>
#include <drivers/mouse.h>
#include <drivers/keyboard.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>
#include <memory/heap.h>
#include <drivers/pit.h>
#include <kcon.h>

static draw_context_t *g_fb_ctx;
static draw_context_t *g_bb_ctx;
static bool g_change = true;

static kdesktop_window_t *head = 0;

int g_mouse_x = 0;
int g_mouse_y = 0;

kdesktop_window_t *g_mouse_dragging;
int g_mouse_dragging_offset_x;
int g_mouse_dragging_offset_y;

static void draw_desktop() {
    draw_rect(g_bb_ctx, 0, 0, g_bb_ctx->width, g_bb_ctx->height, draw_color(g_bb_ctx, 112, 40, 212));
    kdesktop_window_t *window = head;
    while(window) {
        draw_rect(g_bb_ctx, window->x, window->y, window->ctx->width, 16, draw_color(g_bb_ctx, 170, 170, 170));
        for(uint16_t y = 0; y < window->ctx->height; y++) {
            if(window->y + y + 16 < 0) continue;
            if(window->y + y + 16 >= g_bb_ctx->height) break;
            uint16_t width = window->ctx->width;
            if(window->x + width > g_bb_ctx->width) width = g_bb_ctx->width - window->x;
            int x_offset = window->x < 0 ? -window->x : 0;
            memcpy(g_bb_ctx->address + ((window->y + y + 16) * g_bb_ctx->pitch + window->x + x_offset) * sizeof(draw_color_t), window->ctx->address + (y * window->ctx->pitch + x_offset) * sizeof(draw_color_t), (width - x_offset) * sizeof(draw_color_t));
        }

        int i = 0;
        while(window->title[i]) {
            draw_char(g_bb_ctx, window->x + i * BASICFONT_WIDTH, window->y, window->title[i], 0);
            i++;
        }
        window = window->next;
    }
}

static void mouse_handler(int16_t x, int16_t y, bool buttons[3]) {
    for(int xx = 0; xx < 8; xx++) {
        for(int yy = 0; yy < 8; yy++) {
            draw_pixel(g_fb_ctx, g_mouse_x + xx, g_mouse_y + yy, draw_getpixel(g_bb_ctx, g_mouse_x + xx, g_mouse_y + yy));
        }
    }
    int new_x = g_mouse_x + x;
    int new_y = g_mouse_y + -y;
    if(new_x < 0) new_x = 0;
    if(new_y < 0) new_y = 0;
    if(new_x >= g_fb_ctx->width - 8) new_x = g_fb_ctx->width - 8;
    if(new_y >= g_fb_ctx->height - 16) new_y = g_fb_ctx->height - 16;
    g_mouse_x = new_x;
    g_mouse_y = new_y;
    draw_rect(g_fb_ctx, g_mouse_x, g_mouse_y, 6, 4, 0);
    draw_rect(g_fb_ctx, g_mouse_x, g_mouse_y, 8, 2, 0);
    draw_rect(g_fb_ctx, g_mouse_x, g_mouse_y, 4, 6, 0);
    draw_rect(g_fb_ctx, g_mouse_x, g_mouse_y, 2, 8, 0);

    if(buttons[0]) {
        if(g_mouse_dragging) {
            g_mouse_dragging->x = g_mouse_x - g_mouse_dragging_offset_x;
            g_mouse_dragging->y = g_mouse_y - g_mouse_dragging_offset_y;
            g_change = true;
        } else {
            kdesktop_window_t *window = head;
            while(window) {
                if(g_mouse_x >= window->x && g_mouse_x < window->x + window->ctx->width && g_mouse_y >= window->y && g_mouse_y < window->y + 16) {
                    g_mouse_dragging = window;
                    g_mouse_dragging_offset_x = g_mouse_x - window->x;
                    g_mouse_dragging_offset_y = g_mouse_y - window->y;
                }
                window = window->next;
            }
        }
    } else {
        g_mouse_dragging = 0;
    }
}

draw_context_t *kdesktop_create_window(int x, int y, uint16_t width, uint16_t height, char* title) {
    kdesktop_window_t *window = heap_alloc(sizeof(kdesktop_window_t));
    window->title = title;
    window->x = x;
    window->y = y;
    draw_context_t *window_ctx = heap_alloc(sizeof(draw_context_t));
    window_ctx->width = width;
    window_ctx->pitch = window_ctx->width;
    window_ctx->height = height;
    window_ctx->address = heap_alloc(window_ctx->height * window_ctx->pitch * sizeof(draw_color_t));
    window_ctx->colormask = g_bb_ctx->colormask;
    window_ctx->invalidated = true;
    window->ctx = window_ctx;
    window->next = head;
    head = window;
    g_change = true;
    return window_ctx;
}

void kdesktop_initialize(draw_context_t *ctx) {
    g_fb_ctx = ctx;
    g_bb_ctx = heap_alloc(sizeof(draw_context_t));
    memcpy(g_bb_ctx, g_fb_ctx, sizeof(draw_context_t));
    g_bb_ctx->address = heap_alloc(g_bb_ctx->height * g_bb_ctx->pitch * sizeof(draw_color_t));

    kcon_initialize(kdesktop_create_window(50, 50, 800, 500, "Terminal"));
    keyboard_set_handler(kcon_keyboard_handler);
    mouse_set_handler(mouse_handler);

    while(true) {
        if(g_change) {
            draw_desktop();
            g_change = false;
            uint64_t count = g_fb_ctx->pitch * g_fb_ctx->height * sizeof(draw_color_t) / 8;
            uint64_t dst = (uint64_t) g_fb_ctx->address;
            uint64_t src = (uint64_t) g_bb_ctx->address;
            asm volatile("cld\n rep movsq" : "+D" (dst), "+S" (src), "+c" (count) : : "memory");
            // TODO: Deal with leftovers and non 64bit alignment
        }

        kdesktop_window_t *window = head;
        while(window) {
            if(window->ctx->invalidated) {
                for(uint16_t y = 0; y < window->ctx->height; y++) {
                    if(window->y + y + 16 >= g_bb_ctx->height) break;
                    uint16_t width = window->ctx->width;
                    if(window->x + width > g_bb_ctx->width) width = g_bb_ctx->width - window->x;
                    memcpy(g_bb_ctx->address + ((window->y + y + 16) * g_bb_ctx->pitch + window->x) * sizeof(draw_color_t), window->ctx->address + y * window->ctx->pitch * sizeof(draw_color_t), width * sizeof(draw_color_t));
                    memcpy(g_fb_ctx->address + ((window->y + y + 16) * g_fb_ctx->pitch + window->x) * sizeof(draw_color_t), window->ctx->address + y * window->ctx->pitch * sizeof(draw_color_t), width * sizeof(draw_color_t));
                }
                window->ctx->invalidated = false;
            }
            window = window->next;
        }
    }
}