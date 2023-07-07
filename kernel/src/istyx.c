#include "istyx.h"
#include <lib/kprint.h>
#include <string.h>
#include <graphics/basicfont.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <drivers/pci.h>

#define PREFIX "> "
#define TAB_WIDTH 4
#define SCR_INDENT 25
#define MAX_CHARS 512

static draw_color_t g_bg, g_fg;
static draw_context_t *g_ctx;
static int g_x, g_y;

static char g_chars[MAX_CHARS];
static int g_chars_written = 0;

static void clear() {
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, draw_color(20, 20, 25));
    g_x = SCR_INDENT;
    g_y = SCR_INDENT;
}

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

static bool get_arg(char *str, int argc, char **value) {
    int cur = 0;
    while(*str && cur != argc) {
        if(is_whitespace(*str)) {
            while(is_whitespace(*str)) str++;
            cur++;
        } else str++;
    }
    if(cur != argc) return true;

    int strlen = 0;
    while(str[strlen] && str[strlen] != ' ') strlen++;
    if(strlen <= 0) return true;
    *value = heap_alloc(strlen + 1);
    memcpy(*value, str, strlen);
    (*value)[strlen] = 0;
    return false;
}

static bool get_arg_num(char *str, int argc, uint64_t *value) {
    char *arg;
    if(get_arg(str, argc, &arg)) return true;
    int argl = strlen(arg);
    *value = 0;
    uint64_t pow = 1;
    if(arg[0] == '0' && arg[1] == 'x') {
        for(int i = argl - 1; i >= 2; i--) {
            bool valid = false;
            if(arg[i] >= '0' && arg[i] <= '9') { *value += (arg[i] - '0') * pow; valid = true; }
            if(arg[i] >= 'a' && arg[i] <= 'f') { *value += (arg[i] - 'a' + 10) * pow; valid = true; }
            if(arg[i] >= 'A' && arg[i] <= 'F') { *value += (arg[i] - 'A' + 10) * pow; valid = true; }
            if(!valid) {
                heap_free(arg);
                return true;
            }
            pow *= 16;
        }
    } else {
        for(int i = argl - 1; i >= 0; i--) {
            if(arg[i] < '0' || arg[i] > '9') {
                heap_free(arg);
                return true;
            }
            *value += ((uint64_t) arg[i] - '0') * pow;
            pow *= 10;
        }
    }
    heap_free(arg);
    return false;
}

static void command_handler(char *input) {
    char *s = input;
    bool last_was_space = false;
    int arg_count = 1;
    while(*s++) {
        if(is_whitespace(*s)) {
            if(!last_was_space) arg_count++;
            last_was_space = true;
        } else last_was_space = false;
    }

    char *command = "";
    get_arg(input, 0, &command);
    if(command[0]) {
        if(strcmp(command, "clear") == 0) {
            clear();
        } else if(strcmp(command, "pcidev") == 0) {
            pci_device_t *device = g_pci_devices;
            while(device) {
                uint16_t vendor_id = pci_config_read_word(device, __builtin_offsetof(pci_device_header_t, vendor_id));
                uint8_t class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, class));
                uint8_t sub_class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, sub_class));
                uint8_t prog_if = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, program_interface));
                if(device->segment > 0) kprintf("Seg %i ", device->segment);
                kprintf("%i:%i.%i\tVendor: %#x, Class: %#x, SubClass: %#x, ProgIf: %#x\n", device->bus, device->slot, device->func, vendor_id, class, sub_class, prog_if);
                device = device->list;
            }
        } else if(strcmp(command, "hexdump") == 0) {
            if(arg_count < 3) {
                kprintf("Missing arguments\n");
            } else {
                uint64_t usephys;
                uint64_t address;
                uint64_t count;
                if(get_arg_num(input, 3, &usephys) || usephys > 1) usephys = 0;
                if(get_arg_num(input, 1, &address) || get_arg_num(input, 2, &count)) {
                    kprintf("Invalid arguments");
                } else {
                    if(usephys) address = HHDM(address);
                    bool star = false;
                    uint8_t last[10];
                    int row_count = ((int) count + 9) / 10;
                    for(int y = 0; y < row_count; y++) {
                        int row_length = (int) count - y * 10;
                        if(row_length > 10) row_length = 10;
                        bool identical = (y != 0 && y != row_count - 1);
                        if(identical) {
                            for(int x = 0; x < row_length; x++) {
                                uint8_t value = *(uint8_t *) (address + y * 10 + x);
                                if(last[x] != value) identical = false;
                                last[x] = value;
                            }
                        }

                        if(!identical) {
                            star = false;
                            kprintf("%#18.16lx:  ", address + y * 10);
                            for(int x = 0; x < row_length; x++) {
                                kprintf("%.2x ", *(uint8_t *) (address + y * 10 + x));
                            }
                            for(int x = row_length; x < 10; x++) kprintf("   ");

                            kprintf("  ");
                            for(int x = 0; x < row_length; x++) {
                                char c = *(char *) (address + y * 10 + x);
                                if(c < 32 || c > 126) c = '.';
                                kprintf("%c ", c);
                            }
                            kprintf("\n");
                        } else {
                            if(star) continue;
                            kprintf("*\n");
                            star = true;
                        }
                    }
                }
            }
        } else if(strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
            kprintf(
                "Integrated Styx Help\n"
                "\tclear - Clears the screen\n"
                "\tpcidev - Displays the PCI devices\n"
                "\thexdump <address> <count> [physical] - Dumps memory\n"
            );
        } else {
            kprintf("Unknown command: \"%s\"\n", command);
        }
    }

    heap_free(command);
}

void istyx_early_initialize(draw_context_t *draw_context) {
    g_ctx = draw_context;
    g_x = SCR_INDENT;
    g_y = SCR_INDENT;
    g_bg = draw_color(20, 20, 25);
    g_fg = draw_color(230, 230, 230);
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, g_bg);
}

void istyx_simple_input(uint8_t ch) {
    switch(ch) {
        case 0: return;
        case '\b':
            if(g_chars_written <= 0) return;
            g_chars_written--;
            break;
        case '\n':
            putchar('\n');
            g_chars[g_chars_written] = 0;
            command_handler(g_chars);
            g_chars_written = 0;
            kprintf("%s", PREFIX);
            return;
        default:
            if(g_chars_written > MAX_CHARS) return;
            g_chars[g_chars_written] = ch;
            g_chars_written++;
            break;
    }
    putchar(ch);
}

void istyx_thread_init() {
    kprintf(" _____ _         _           _____ _____ \n");
    kprintf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    kprintf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    kprintf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    kprintf("        |___|                            \n\n");
    kprintf("Welcome to Integrated Styx V1.0 running on Elysium OS\n");
    kprintf("%s", PREFIX);

    while(true);
}

int putchar(int c) {
    switch(c) {
        case '\t':
            g_x += (TAB_WIDTH - (g_x / BASICFONT_WIDTH) % TAB_WIDTH) * BASICFONT_WIDTH;
            break;
        case '\b':
            g_x -= BASICFONT_WIDTH;
            if(g_x < SCR_INDENT) g_x = SCR_INDENT;
            draw_rect(g_ctx, g_x, g_y, BASICFONT_WIDTH, BASICFONT_HEIGHT, g_bg);
            break;
        case '\n':
            g_x = SCR_INDENT;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(g_ctx, g_x, g_y, (char) c, g_fg);
            g_x += BASICFONT_WIDTH;
            break;
    }
    if(g_x >= g_ctx->width - SCR_INDENT) {
        g_x = SCR_INDENT;
        g_y += BASICFONT_HEIGHT;
    }
    if(g_y >= g_ctx->height - SCR_INDENT) {
    }
    return (char) c;
}