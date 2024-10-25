#include "graphics/draw.h"
#include "graphics/font.h"
#include "format.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/utsname.h>
#include <elib.h>

#define DEFAULT_FG 0xFFFFFFFF
#define DEFAULT_BG 0
#define TAB_WIDTH 4
#define PREFIX '>'
#define MAX_CHARS 512

static int g_cursor_x, g_cursor_y;
static draw_context_t g_ctx;
static font_t g_font;

static char g_chars[MAX_CHARS];
static int g_chars_written = 0;
static char g_path[PATH_MAX];
static int g_path_length = 0;

void conprint(const char *fmt, ...);

static void clear() {
    draw_rect(&g_ctx, 0, 0, g_ctx.width, g_ctx.height, DEFAULT_BG);
    g_cursor_x = 0;
    g_cursor_y = 0;
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
    *value = malloc(strlen + 1);
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
                free(arg);
                return true;
            }
            pow *= 16;
        }
    } else {
        for(int i = argl - 1; i >= 0; i--) {
            if(arg[i] < '0' || arg[i] > '9') {
                free(arg);
                return true;
            }
            *value += ((uint64_t) arg[i] - '0') * pow;
            pow *= 10;
        }
    }
    free(arg);
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

    char *command;
    get_arg(input, 0, &command);
    if(command[0]) {
        if(strcmp(command, "clear") == 0) {
            clear();
        } else if(strcmp(command, "time") == 0) {
            struct timespec tp;
            assert(clock_getres(CLOCK_REALTIME, &tp) == 0);
            conprint("Realtime resolution: %lu, %lu\n", tp.tv_sec, tp.tv_nsec);

            assert(clock_gettime(CLOCK_REALTIME, &tp) == 0);
            conprint("Realtime: %lu, %lu\n", tp.tv_sec, tp.tv_nsec);
        } else if(strcmp(command, "uname") == 0) {
            struct utsname name;
            assert(uname(&name) == 0);
            conprint("%s (%s %s)\n", name.sysname, name.nodename, name.version);
        } else if(strcmp(command, "exec") == 0) {
            if(arg_count < 2) {
                conprint("Missing arguments\n");
            } else {
                char *path;
                if(get_arg(input, 1, &path)) {
                    conprint("Invalid argument(s)\n");
                } else {
                    // FILE *f = fopen(path, "")
                }
            }

        // } else if(strcmp(command, "cd") == 0) {
        //     char *str;
        //     if(arg_count < 2) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         if(get_arg(input, 1, &str)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             vfs_node_t *node;
        //             if(vfs_lookup(g_cwd, str, &node) == 0 && node->type == VFS_NODE_TYPE_DIRECTORY) {
        //                 g_cwd = node;

        //                 char *cstr = str;
        //                 if(*str == '/') g_path_length = 0;

        //                 bool term_start = true;
        //                 while(*cstr) {
        //                     switch(*cstr) {
        //                         case '/':
        //                             term_start = true;
        //                             cstr++;
        //                             continue;
        //                         case '.':
        //                             if(!term_start) break;
        //                             int period_count = 1;
        //                             if(cstr[1] == '.') period_count = 2;
        //                             if(cstr[period_count] && cstr[period_count] != '/') break;
        //                             if(period_count == 2) {
        //                                 while(g_path[g_path_length] != '/' && g_path_length > 0) g_path_length--;
        //                                 g_path[g_path_length] = 0;
        //                             }
        //                             cstr += period_count;
        //                             continue;
        //                     }
        //                     if(term_start && g_path_length > 1) g_path[g_path_length++] = '/';
        //                     term_start = false;
        //                     g_path[g_path_length++] = *cstr++;
        //                 }
        //                 if(!g_path_length) g_path[g_path_length++] = '/';
        //             }
        //             heap_free(str);
        //         }
        //     }
        // } else if(strcmp(command, "mkdir") == 0) {
        //     char *str;
        //     if(arg_count < 2) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         if(get_arg(input, 1, &str)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             vfs_node_t *tmp;
        //             vfs_node_attributes_t *attributes = heap_alloc(sizeof(vfs_node_attributes_t));
        //             attributes->type = VFS_NODE_TYPE_DIRECTORY;
        //             if(g_cwd->ops->create(g_cwd, &tmp, str, attributes)) conprint("Failed\n");
        //             heap_free(attributes);
        //             heap_free(str);
        //         }
        //     }
        // } else if(strcmp(command, "ls") == 0) {
        //     off_t offset = 0;
        //     dirent_t *dirent = heap_alloc(sizeof(dirent_t));
        //     while((offset = g_cwd->ops->readdir(g_cwd, dirent, offset))) {
        //         conprint("%s\n", dirent->d_name);
        //     }
        //     heap_free(dirent);
        // } else if(strcmp(command, "pcidev") == 0) {
        //     pci_device_t *device = g_pci_devices;
        //     while(device) {
        //         uint16_t vendor_id = pci_config_read_word(device, __builtin_offsetof(pci_device_header_t, vendor_id));
        //         uint8_t class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, class));
        //         uint8_t sub_class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, sub_class));
        //         uint8_t prog_if = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, program_interface));
        //         if(device->segment > 0) conprint("Seg %i ", device->segment);
        //         conprint("%i:%i.%i Vendor: %x, Class: %x, SubClass: %x, ProgIf: %x\n", device->bus, device->slot, device->func, vendor_id, class, sub_class, prog_if);
        //         device = device->list;
        //     }
        // } else if(strcmp(command, "rpage") == 0) {
        //     conprint("Requested 1 page >> %#lx\n", pmm_page_request());
        // } else if(strcmp(command, "reqcpage") == 0) {
        //     uint64_t pages;
        //     if(arg_count < 2) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         if(get_arg_num(input, 1, &pages)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             conprint("Requested %li pages >> %#lx\n", pages, pmm_lowmem_request(pages));
        //         }
        //     }
        // } else if(strcmp(command, "relcpage") == 0) {
        //     uint64_t address;
        //     uint64_t pages;
        //     if(arg_count < 3) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         if(get_arg_num(input, 1, &address) || get_arg_num(input, 2, &pages)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             pmm_lowmem_release((void *) address, pages);
        //             conprint("Released %li pages starting at %#lx\n", pages, address);
        //         }
        //     }
        // } else if(strcmp(command, "read") == 0) {
        //     if(arg_count < 4) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         uint64_t sector;
        //         uint64_t count;
        //         uint64_t dest;
        //         if(get_arg_num(input, 1, &sector) || get_arg_num(input, 2, &count) || get_arg_num(input, 3, &dest)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             ahci_read(0, sector, count, (void *) dest);
        //             conprint("Read %li sectors starting at %li into %#lx\n", count, sector, dest);
        //         }
        //     }
        // } else if(strcmp(command, "phys") == 0) {
        //     if(arg_count < 2) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         uint64_t virt;
        //         if(get_arg_num(input, 1, &virt)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             conprint("Physical %#lx\n", vmm_physical((void *) virt));
        //         }
        //     }
        // } else if(strcmp(command, "vdbg") == 0) {
        //     if(arg_count < 5) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         uint64_t indexes[4];
        //         if(get_arg_num(input, 1, indexes) || get_arg_num(input, 2, indexes + 1) || get_arg_num(input, 3, indexes + 2) || get_arg_num(input, 4, indexes + 3)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             uint64_t entries[4] = {0};
        //             vmm_dbg_tables(indexes, entries);
        //             for(int i = 0; i < 4; i++) {
        //                 uint64_t address = entries[i] & 0x000FFFFFFFFFF000;
        //                 uint64_t flags = entries[i] & 0xFFF;
        //                 conprint("L%i Entry [%#lx] (%#lx)\n", 4 - i, address, flags);
        //             }
        //         }
        //     }
        // } else if(strcmp(command, "reqvpage") == 0) {
        //     if(arg_count < 3) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         uint64_t virt;
        //         uint64_t page_count;
        //         if(get_arg_num(input, 1, &virt) || get_arg_num(input, 2, &page_count)) {
        //             conprint("Invalid arguments\n");
        //         } else {
        //             for(uint64_t i = 0; i < page_count; i++) {
        //                 vmm_map(pmm_page_request(), (void *) (virt + i * 0x1000));
        //             }
        //             conprint("Allocated %li pages at virtual address %#lx\n", page_count, virt);
        //         }
        //     }
        // } else if(strcmp(command, "hexdump") == 0) {
        //     if(arg_count < 3) {
        //         conprint("Missing arguments\n");
        //     } else {
        //         uint64_t usephys;
        //         uint64_t address;
        //         uint64_t count;
        //         if(get_arg_num(input, 3, &usephys) || usephys > 1) usephys = 0;
        //         if(get_arg_num(input, 1, &address) || get_arg_num(input, 2, &count)) {
        //             conprint("Invalid arguments");
        //         } else {
        //             if(usephys) address = HHDM(address);
        //             bool star = false;
        //             uint8_t last[10];
        //             int row_count = ((int) count + 9) / 10;
        //             for(int y = 0; y < row_count; y++) {
        //                 int row_length = (int) count - y * 10;
        //                 if(row_length > 10) row_length = 10;
        //                 bool identical = (y != 0 && y != row_count - 1);
        //                 if(identical) {
        //                     for(int x = 0; x < row_length; x++) {
        //                         uint8_t value = *(uint8_t *) (address + y * 10 + x);
        //                         if(last[x] != value) identical = false;
        //                         last[x] = value;
        //                     }
        //                 }

        //                 if(!identical) {
        //                     star = false;
        //                     conprint("%#18.16lx:  ", address + y * 10);
        //                     for(int x = 0; x < row_length; x++) {
        //                         conprint("%.2x ", *(uint8_t *) (address + y * 10 + x));
        //                     }
        //                     for(int x = row_length; x < 10; x++) conprint("   ");

        //                     conprint("  ");
        //                     for(int x = 0; x < row_length; x++) {
        //                         char c = *(char *) (address + y * 10 + x);
        //                         if(c < 32 || c > 126) c = '.';
        //                         conprint("%c ", c);
        //                     }
        //                     conprint("\n");
        //                 } else {
        //                     if(star) continue;
        //                     conprint("*\n");
        //                     star = true;
        //                 }
        //             }
        //         }
        //     }
        } else if(strcmp(command, "help") == 0) {
            conprint("clear: Clear the console\n");
            conprint("uname: Display uname\n");
            conprint("time: Display time information\n");
        //     conprint("pcidev: Display all active PCI devices\n");
        //     conprint("rpage: Get one page of memory\n");
        //     conprint("rcpage: Get contiguous pages of memory\n");
        //     conprint("read: Reads from disk\n");
        //     conprint("hexdump: Dumps memory at an address\n");
            conprint("help: Display all the commands\n");
        } else {
            conprint("Unknown command: \"%s\"\n", command);
        }
    }

    free(command);
}

void conwrite(char c) {
    switch(c) {
        case '\n':
            g_cursor_x = 0;
            g_cursor_y += g_font.height;
            break;
        case '\t':
            g_cursor_x += (TAB_WIDTH - (g_cursor_x / g_font.width) % TAB_WIDTH) * g_font.width;
            break;
        case '\b':
            g_cursor_x -= g_font.width;
            if(g_cursor_x < 0) g_cursor_x = 0;
            draw_rect(&g_ctx, g_cursor_x, g_cursor_y, g_font.width, g_font.height, DEFAULT_BG);
            break;
        default:
            draw_char(&g_ctx, g_cursor_x, g_cursor_y, c, &g_font, DEFAULT_FG);
            g_cursor_x += g_font.width;
            break;
    }
    if(g_cursor_x + g_font.width >= g_ctx.width) {
        g_cursor_x = 0;
        g_cursor_y += g_font.height;
    }
    if(g_cursor_y + g_font.height >= g_ctx.height) clear();
}

void kcon_keyboard_handler(uint8_t character) {
    switch(character) {
        case 0: return;
        case '\b':
            if(g_chars_written <= 0) return;
            g_chars_written--;
            break;
        case '\n':
            conwrite('\n');
            g_chars[g_chars_written] = 0;
            command_handler(g_chars);
            g_chars_written = 0;
            conprint("%.*s %c ", g_path_length, g_path, PREFIX);
            return;
        default:
            if(g_chars_written > MAX_CHARS) return;
            g_chars[g_chars_written] = character;
            g_chars_written++;
            break;
    }
    conwrite(character);
}

void conprint(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    format(conwrite, fmt, list);

    va_end(list);
}

void kcon_initialize() {
    elib_framebuffer_info_t fb_info;
    void *fb = elib_acquire_framebuffer(&fb_info);
    if(fb == NULL) {
        conprint("Failed to acquire the framebuffer\n");
    }
    conprint("Acquired framebuffer\n");

    g_ctx.address = fb;
    g_ctx.height = fb_info.height;
    g_ctx.width = fb_info.width;
    g_ctx.pitch = fb_info.pitch;

    g_font = g_font_basic;

    assert(getcwd(g_path, PATH_MAX) != NULL);
    g_path_length = strlen(g_path);

    draw_rect(&g_ctx, 0, 0, g_ctx.width, g_ctx.height, DEFAULT_BG);
    conprint(" _____ _         _           _____ _____ \n");
    conprint("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    conprint("|   __| | | |_ -| | | |     |  |  |__   |\n");
    conprint("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    conprint("        |___|                            \n\n");
    conprint("Welcome to Elysium OS Shell\n");
    conprint("%.*s %c ", g_path_length, g_path, PREFIX);

    while(true) {
        int ch = elib_input();
        if(ch == -1) continue;
        kcon_keyboard_handler(ch);
    }
}