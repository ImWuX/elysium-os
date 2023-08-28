#include "istyx.h"
#include <string.h>
#include <lib/kprint.h>
#include <lib/list.h>
#include <graphics/basicfont.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <drivers/pci.h>
#include <drivers/ahci.h>

#define PREFIX "> "
#define TAB_WIDTH 4
#define SCR_INDENT 25
#define MAX_CHARS 512

typedef struct arg_constructor {
    char *arg;
    struct arg_constructor *next;
} arg_constructor_t;

typedef union {
    uint64_t u_integer;
    int64_t integer;
    char *string;
    bool boolean;
} arg_t;

typedef enum {
    ARG_STRING,
    ARG_INTEGER,
    ARG_UNSIGNED_INTEGER,
    ARG_BOOLEAN
} command_arg_type_t;

typedef struct {
    char *name;
    command_arg_type_t type;
    bool optional;
} command_arg_t;

typedef struct {
    char *name;
    char *description;
    void (*func)(arg_t *);
    command_arg_t *args;
    int argc;
} command_t;

static draw_color_t g_bg, g_fg, g_cg;

static draw_context_t *g_ctx;

static int g_x, g_y;
static char g_chars[MAX_CHARS];
static int g_chars_written = 0;

static int g_cursor_x = 0, g_cursor_y = 0;
static draw_color_t g_cursor_buffer[22*14];

static int g_cursor_template[22][14] = {
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0},
    {1,1,1,0,0,1,1,1,1,0,0,0,0,0},
    {1,1,0,0,0,1,1,1,1,0,0,0,0,0},
    {1,0,0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,0,0,0,0}
};

static void clear() {
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, g_bg);
    for(int i = 0; i < 22*14; i++) g_cursor_buffer[i] = g_bg;
    g_x = SCR_INDENT;
    g_y = SCR_INDENT;
}

static bool parse_int(char *input, int64_t *out) {
    int64_t value = 0;
    bool negative = false;
    bool hex = input[0] == '0' && (input[0] != 0 && input[1] == 'x');
    if(hex) input += 2;
    if(!hex) for(; *input == '-'; input++) negative = !negative;
    for(; *input; input++) {
        value *= (hex ? 0x10 : 10);
        if(*input >= '0' && *input <= '9') {
            value += *input - '0';
            continue;
        }
        if(hex && *input >= 'a' && *input <= 'f') {
            value += *input - 'a' + 10;
            continue;
        }
        if(hex && *input >= 'A' && *input <= 'F') {
            value += *input - 'A' + 10;
            continue;
        }
        return true;
    }
    if(negative) value *= -1;
    *out = value;
    return false;
}

static bool parse_uint(char *input, uint64_t *out) {
    uint64_t value = 0;
    bool hex = input[0] == '0' && (input[0] != 0 && input[1] == 'x');
    if(hex) input += 2;
    for(; *input; input++) {
        value *= (hex ? 0x10 : 10);
        if(*input >= '0' && *input <= '9') {
            value += *input - '0';
            continue;
        }
        if(hex && *input >= 'a' && *input <= 'f') {
            value += *input - 'a' + 10;
            continue;
        }
        if(hex && *input >= 'A' && *input <= 'F') {
            value += *input - 'A' + 10;
            continue;
        }
        return true;
    }
    *out = value;
    return false;
}

static void command_help(arg_t *args __attribute__((unused)));

static void command_clear(arg_t *args __attribute__((unused))) {
    clear();
}

static void command_pmm_alloc(arg_t *args) {
    if(args[0].integer < 0 || args[0].integer > PMM_MAX_ORDER) return (void) kprintf("Invalid order\n");
    pmm_page_t *page = pmm_alloc(args[0].integer, PMM_GENERAL);
    kprintf("Order %li^2 page(%#lx) >> %#lx\n", args[0].integer, (uint64_t) page, page->paddr);
}

static void command_pmm_free(arg_t *args) {
    pmm_page_t *page = (pmm_page_t *) args[0].u_integer;
    uint8_t order = page->order;
    pmm_free(page);
    kprintf("Freed order %u^2 page\n", order);
}

static void command_heap_alloc(arg_t *args) {
    uint64_t block;
    if(args[1].u_integer) {
        block = (uintptr_t) heap_alloc_align(args[0].u_integer, args[1].u_integer);
    } else {
        block = (uintptr_t) heap_alloc(args[0].u_integer);
    }
    kprintf("Address %#lx\n", block);
}

static void command_heap_free(arg_t *args) {
    heap_free((void *) args[0].u_integer);
}

static void command_read(arg_t *args) {
    ahci_read(0, args[0].u_integer, args[1].u_integer, (void *) args[2].u_integer);
    kprintf("Read %li sectors starting at %li into %#lx\n", args[1].u_integer, args[0].u_integer, args[2].u_integer);
}

static void command_meminfo(arg_t *args __attribute__((unused))) {
    for(int i = 0; i < PMM_ZONE_COUNT; i++) {
        pmm_zone_t *zone = &g_pmm_zones[i];
        kprintf("| Zone (%s) %#lx pages\n", zone->name, zone->page_count);
        list_t *entry;
        LIST_FOREACH(entry, &zone->regions) {
            pmm_region_t *region = LIST_GET(entry, pmm_region_t, list);
            kprintf("\t| Region %#lx pages\n", region->page_count);
        }
    }
}

static void command_pcidev(arg_t *args __attribute__((unused))) {
    list_t *entry;
    LIST_FOREACH(entry, &g_pci_devices) {
        pci_device_t *device = LIST_GET(entry, pci_device_t, list);
        uint16_t vendor_id = pci_config_read_word(device, offsetof(pci_device_header_t, vendor_id));
        uint8_t class = pci_config_read_byte(device, offsetof(pci_device_header_t, class));
        uint8_t sub_class = pci_config_read_byte(device, offsetof(pci_device_header_t, sub_class));
        uint8_t prog_if = pci_config_read_byte(device, offsetof(pci_device_header_t, program_interface));
        if(device->segment > 0) kprintf("Seg %i ", device->segment);
        kprintf("%i:%i.%i\tVendor: %#x, Class: %#x, SubClass: %#x, ProgIf: %#x\n", device->bus, device->slot, device->func, vendor_id, class, sub_class, prog_if);
    }
}

static void command_hexdump(arg_t *args) {
    uint64_t address = args[0].u_integer;
    if(args[2].u_integer) address = HHDM(address);
    bool star = false;
    uint8_t last[10];
    int row_count = ((int) args[1].u_integer + 9) / 10;
    for(int y = 0; y < row_count; y++) {
        int row_length = (int) args[1].u_integer - y * 10;
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

static command_t g_command_registry[] = {
    {
        .name = "help",
        .description = "Lists registered commands and their usages",
        .func = &command_help
    },
    {
        .name = "clear",
        .description = "Clears the screen",
        .func = &command_clear,
    },
    {
        .name = "pmm-alloc",
        .description = "Uses the PMM directly to allocate physical memory",
        .func = &command_pmm_alloc,
        .args = (command_arg_t[]) {
            { .name = "order", .type = ARG_INTEGER }
        },
        .argc = 1
    },
    {
        .name = "pmm-free",
        .description = "Uses the PMM directly to free physical memory",
        .func = &command_pmm_free,
        .args = (command_arg_t[]) {
            { .name = "page address", .type = ARG_UNSIGNED_INTEGER }
        },
        .argc = 1
    },
    {
        .name = "heap-alloc",
        .description = "Uses the heap to allocate memory",
        .func = &command_heap_alloc,
        .args = (command_arg_t[]) {
            { .name = "byte count", .type = ARG_UNSIGNED_INTEGER },
            { .name = "alignment", .type = ARG_UNSIGNED_INTEGER, .optional = true }
        },
        .argc = 2
    },
    {
        .name = "heap-free",
        .description = "Uses the heap to free memory",
        .func = &command_heap_free,
        .args = (command_arg_t[]) {
            { .name = "address", .type = ARG_UNSIGNED_INTEGER }
        },
        .argc = 1
    },
    {
        .name = "read",
        .description = "Reads memory from disk",
        .func = &command_read,
        .args = (command_arg_t[]) {
            { .name = "LBA", .type = ARG_UNSIGNED_INTEGER },
            { .name = "sector count", .type = ARG_UNSIGNED_INTEGER },
            { .name = "destination", .type = ARG_UNSIGNED_INTEGER }
        },
        .argc = 3
    },
    {
        .name = "meminfo",
        .description = "Lists physical memory region availability",
        .func = &command_meminfo
    },
    {
        .name = "pcidev",
        .description = "Lists all discovered PCI devices",
        .func = &command_pcidev
    },
    {
        .name = "hexdump",
        .description = "Displays physical or virtual memory",
        .func = &command_hexdump,
        .args = (command_arg_t[]) {
            { .name = "address", .type = ARG_UNSIGNED_INTEGER },
            { .name = "count", .type = ARG_UNSIGNED_INTEGER },
            { .name = "physical address", .type = ARG_BOOLEAN, .optional = true }
        }
    }
};

static void command_help(arg_t *args __attribute__((unused))) {
    kprintf("Integrated Styx | Help\n");
    for(unsigned int cmd_idx = 0; cmd_idx < sizeof(g_command_registry) / sizeof(command_t); cmd_idx++) {
        kprintf("\t%s", g_command_registry[cmd_idx].name);
        for(int arg_idx = 0; arg_idx < g_command_registry[cmd_idx].argc; arg_idx++) {
            kprintf(g_command_registry[cmd_idx].args[arg_idx].optional ? " [%s]" : " <%s>", g_command_registry[cmd_idx].args[arg_idx].name);
        }
        kprintf(" - %s\n", g_command_registry[cmd_idx].description);
    }
}

static void command_handler(char *input) {
    if(!input[0]) return;

    int constructor_length = 0;
    arg_constructor_t *constructor = 0;
    bool escaped = false;
    for(int i = 0, start = 0;; i++) {
        switch(input[i]) {
            case '"':
                escaped = !escaped;
                break;
            case 0:
            case ' ':
                if(!escaped) break;
            default:
                goto end;
        }
        if(i - start > 0) {
            int segment_length = i - start;
            char *segment = heap_alloc(segment_length + 1);
            memcpy(segment, input + start, segment_length);
            segment[segment_length] = 0;

            arg_constructor_t *entry = heap_alloc(sizeof(arg_constructor_t));
            entry->arg = segment;
            entry->next = constructor;
            constructor = entry;
            constructor_length++;
        }
        start = i + 1;

        end:
        if(input[i] == 0) break;
    }
    if(constructor_length == 0) return;

    constructor_length--;
    char **strargs = heap_alloc(sizeof(char *) * constructor_length);
    for(int i = constructor_length - 1; i >= 0; i--) {
        strargs[i] = constructor->arg;
        arg_constructor_t *next = constructor->next;
        heap_free(constructor);
        constructor = next;
    }
    char *command_str = constructor->arg;
    heap_free(constructor);

    command_t *command = 0;
    for(unsigned int i = 0; i < sizeof(g_command_registry) / sizeof(command_t); i++) {
        if(strcmp(g_command_registry[i].name, command_str) != 0) continue;
        command = &g_command_registry[i];
    }

    if(command) {
        arg_t *args = heap_alloc(sizeof(arg_t) * command->argc);
        for(int i = 0; i < command->argc; i++) {
            if(i >= constructor_length) {
                if(!command->args[i].optional) {
                    kprintf("Missing non-optional paramater(s)\n");
                    goto invalid;
                } else {
                    memset(&args[i], 0, sizeof(arg_t));
                }
            } else {
                switch((command->argc <= i) ? ARG_STRING : command->args[i].type) {
                    case ARG_STRING:
                        args[i].string = strargs[i];
                        break;
                    case ARG_INTEGER:
                        if(parse_int(strargs[i], &args[i].integer)) {
                            kprintf("Invalid parameter, %s is not an integer\n", strargs[i]);
                            goto invalid;
                        }
                        break;
                    case ARG_UNSIGNED_INTEGER:
                        if(parse_uint(strargs[i], &args[i].u_integer)) {
                            kprintf("Invalid parameter, %s is not an unsigned integer\n", strargs[i]);
                            goto invalid;
                        }
                        break;
                    case ARG_BOOLEAN:
                        args[i].boolean = strcmp(strargs[i], "true") == 0 || strcmp(strargs[i], "1") == 0;
                        break;
                }
            }
        }
        command->func(args);

        invalid:
        heap_free(args);
    } else {
        kprintf("Unknown command: \"%s\"\n", command_str);
    }

    for(int i = 0; i < constructor_length; i++) heap_free(strargs[i]);
    heap_free(strargs);
}

void istyx_early_initialize(draw_context_t *draw_context) {
    g_ctx = draw_context;
    g_bg = draw_color(20, 20, 25);
    g_fg = draw_color(230, 230, 230);
    g_cg = draw_color(252, 186, 3);
    clear();
}

void istyx_simple_input_kb(uint8_t ch) {
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

void istyx_simple_input_mouse(int16_t rel_x, int16_t rel_y, bool buttons[3]) {
    for(int x = 0; x < 14; x++) {
        for(int y = 0; y < 22; y++) {
            if(g_cursor_template[y][x]) draw_pixel(g_ctx, g_cursor_x + x, g_cursor_y + y, g_cursor_buffer[y * 14 + x]);
            if(x > 3 || y > 3) continue;
            if(buttons[2]) draw_pixel(g_ctx, g_cursor_x + x, g_cursor_y + y, g_cg);
        }
    }
    g_cursor_x += rel_x;
    g_cursor_y += -rel_y;
    if(g_cursor_x < 0) g_cursor_x = 0;
    if(g_cursor_y < 0) g_cursor_y = 0;
    if(g_cursor_x >= g_ctx->width) g_cursor_x = g_ctx->width - 1;
    if(g_cursor_y >= g_ctx->height) g_cursor_y = g_ctx->height - 1;

    for(int x = 0; x < 14; x++) {
        for(int y = 0; y < 22; y++) {
            if(!g_cursor_template[y][x]) continue;
            g_cursor_buffer[y * 14 + x] = draw_getpixel(g_ctx, g_cursor_x + x, g_cursor_y + y);
            draw_pixel(g_ctx, g_cursor_x + x, g_cursor_y + y, g_cg);
        }
    }
}

void istyx_thread_init() {
    kprintf(" _____ _         _           _____ _____ \n");
    kprintf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    kprintf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    kprintf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    kprintf("        |___|                            \n\n");
    kprintf("Welcome to Integrated Styx V1.0 running on Elysium OS\n");
    kprintf("%s", PREFIX);

    for(;;);
}

int putchar(int c) {
    switch(c) {
        case '\t':
            g_x += (TAB_WIDTH - ((g_x - SCR_INDENT) / BASICFONT_WIDTH) % TAB_WIDTH) * BASICFONT_WIDTH;
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