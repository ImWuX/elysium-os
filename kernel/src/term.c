#include "term.h"
#include <lib/format.h>
#include <common/log.h>
#include <graphics/font.h>

static struct {
    draw_context_t *context;
    font_t *font;
    int ch_width, ch_height;
    struct {
        draw_color_t bg, fg;
        draw_color_t debug, info, warn, error;
    } colors;
    struct {
        draw_color_t color;
        int x, y;
    } state;
} g_term;

static void set_color(draw_color_t color) {
    g_term.state.color = color;
}

static void clear_term() {
    draw_rect(g_term.context, 0, 0, g_term.context->width, g_term.context->height, g_term.colors.bg);
}

static void draw_char_term(int x, int y, char ch) {
    draw_char(g_term.context, x * g_term.font->width, y * g_term.font->height, ch, g_term.font, g_term.state.color);
}

static void blank_char_term(int x, int y) {
    draw_rect(g_term.context, x * g_term.font->width, y * g_term.font->height, g_term.font->width, g_term.font->height, g_term.colors.bg);
}

static void log_raw_term(char c) {
    switch(c) {
        case '\n':
            g_term.state.x = 0;
            g_term.state.y++;
            break;
        default:
            draw_char_term(g_term.state.x, g_term.state.y, c);
            g_term.state.x++;
            break;
    }
    if(g_term.state.x >= g_term.ch_width) {
        g_term.state.x = 0;
        g_term.state.y++;
    }
    if(g_term.state.y >= g_term.ch_height) {
        clear_term();
        g_term.state.x = 0;
        g_term.state.y = 0;
    }
}

static void format_lrt(char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
    format(log_raw_term, fmt, list);
	va_end(list);
}

static void log_term(log_level_t level, const char *tag, const char *fmt, va_list args) {
    switch(level) {
        case LOG_LEVEL_DEBUG: set_color(g_term.colors.debug); break;
        case LOG_LEVEL_INFO: set_color(g_term.colors.info); break;
        case LOG_LEVEL_WARN: set_color(g_term.colors.warn); break;
        case LOG_LEVEL_ERROR: set_color(g_term.colors.error); break;
        default: set_color(g_term.colors.fg); break;
    }
    format_lrt("[%s::%s] ", log_level_tostring(level), tag);
    set_color(g_term.colors.fg);
    format(log_raw_term, fmt, args);
    log_raw_term('\n');
}

static log_sink_t g_term_sink = {
    .name = "TERM",
    .level = LOG_LEVEL_INFO,
    .log = log_term,
    .log_raw = log_raw_term
};

void term_init(draw_context_t *context) {
    g_term.context = context;
    g_term.font = &g_font_basic;
    g_term.ch_width = context->width / g_term.font->width;
    g_term.ch_height = context->height / g_term.font->height;
    g_term.colors.bg = draw_color(10, 10, 12);
    g_term.colors.fg = draw_color(255, 255, 255);
    g_term.colors.debug = draw_color(18, 121, 255);
    g_term.colors.info = draw_color(252, 218, 23);
    g_term.colors.warn = draw_color(235, 64, 52);
    g_term.colors.error = draw_color(156, 10, 0);
    g_term.state.x = 0;
    g_term.state.y = 0;
    g_term.state.color = g_term.colors.fg;
    log_sink_add(&g_term_sink);
    clear_term();
}

void term_kb_handler(uint8_t ch) {
    switch(ch) {
        case '\b':
            if(g_term.state.x <= 0) break;
            g_term.state.x--;
            blank_char_term(g_term.state.x, g_term.state.y);
            break;
        default:
            log_raw_term(ch);
            break;
    }
}

void term_close() {
    log_sink_remove(&g_term_sink);
    clear_term();
}