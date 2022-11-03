#include "mouse.h"
#include <stdio.h>
#include <drivers/ports.h>
#include <drivers/display.h>
#include <cpu/isr.h>

#define MOUSE_TIMEOUT 100000
#define PS2_STATUS 0x64
#define PS2_DATA 0x60
#define PS2_READY_WRITE 2
#define PS2_READY_READ 1

#define MOUSE_WAIT_WRITE 1
#define MOUSE_WAIT_READ 0

static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static uint32_t last_color[9];

static void mouse_wait(uint8_t type) {
    int timeout = MOUSE_TIMEOUT;
    while(--timeout) {
        if(type == MOUSE_WAIT_READ && (inb(PS2_STATUS) & PS2_READY_READ) == 1) {
            return;
        } else if(type == MOUSE_WAIT_WRITE && !(inb(PS2_STATUS) & PS2_READY_WRITE)) {
            return;
        }
    }
    printe("Mouse timed out");
}

static void mouse_write(uint8_t data) {
	mouse_wait(MOUSE_WAIT_WRITE);
	outb(PS2_STATUS, 0xD4);
	mouse_wait(MOUSE_WAIT_WRITE);
	outb(PS2_DATA, data);
}

static uint8_t mouse_read() {
	mouse_wait(MOUSE_WAIT_READ);
	return inb(PS2_DATA);
}

static void mouse_handler(cpu_register_t registers __attribute__((unused))) {
    uint8_t status = inb(PS2_STATUS);
    while(status & PS2_READY_READ) {
		int8_t mouse_in = inb(PS2_DATA);
		if(status & 0x20) {
			switch(mouse_cycle) {
				case 0:
					mouse_bytes[0] = mouse_in;
					if (!(mouse_in & 0x08)) return;
					++mouse_cycle;
					break;
				case 1:
					mouse_bytes[1] = mouse_in;
					++mouse_cycle;
					break;
				case 2:
					mouse_bytes[2] = mouse_in;
					if(mouse_bytes[0] & 0x80 || mouse_bytes[0] & 0x40) break;

                    for(int px = -1; px < 3; px++) {
                        for(int py = -1; py < 3; py++) {
                            if(mouse_x + px < 0 || mouse_x + px >= get_display_mode_info()->width) continue;
                            if(mouse_y + py < 0 || mouse_y + py >= get_display_mode_info()->height) continue;
                            draw_pixel(mouse_x + px, mouse_y + py, last_color[px + py * 3]);
                        }
                    }
                    int32_t rel_x = mouse_bytes[1] - ((mouse_bytes[0] << 4) & 0x100);
                    int32_t rel_y = mouse_bytes[2] - ((mouse_bytes[0] << 3) & 0x100);
                    mouse_x += rel_x;
                    mouse_y -= rel_y;
                    if(mouse_x >= get_display_mode_info()->width) mouse_x = get_display_mode_info()->width - 1;
                    if(mouse_y >= get_display_mode_info()->height) mouse_y = get_display_mode_info()->height - 1;
                    if(mouse_x < 0) mouse_x = 0;
                    if(mouse_y < 0) mouse_y = 0;
                    for(int px = -1; px < 3; px++) {
                        for(int py = -1; py < 3; py++) {
                            if(mouse_x + px < 0 || mouse_x + px >= get_display_mode_info()->width) continue;
                            if(mouse_y + py < 0 || mouse_y + py >= get_display_mode_info()->height) continue;
                            last_color[px + py * 3] = get_pixel(mouse_x + px, mouse_y + py);
                            draw_pixel(mouse_x + px, mouse_y + py, 0xFFFF0000);
                        }
                    }
					if(mouse_bytes[0] & 0x01) {
					}
					if(mouse_bytes[0] & 0x02) {
					}
					if(mouse_bytes[0] & 0x04) {
                        for(int px = -1; px < 3; px++) {
                            for(int py = -1; py < 3; py++) {
                                if(mouse_x + px < 0 || mouse_x + px >= get_display_mode_info()->width) continue;
                                if(mouse_y + py < 0 || mouse_y + py >= get_display_mode_info()->height) continue;
                                last_color[px + py * 3] = 0xFFFFFFFF;
                            }
                        }
					}
					mouse_cycle = 0;
					break;
			}
		}
		status = inb(PS2_STATUS);
	}
}

void initialize_mouse() {
    mouse_wait(MOUSE_WAIT_WRITE);
    outb(PS2_STATUS, 0xA8);

    mouse_wait(MOUSE_WAIT_WRITE);
    outb(PS2_STATUS, 0x20);
    mouse_wait(MOUSE_WAIT_READ);
    uint8_t status = inb(PS2_DATA);
    status |= 2;
    status &= ~(1 << 5);

    mouse_wait(MOUSE_WAIT_WRITE);
	outb(PS2_STATUS, 0x60);
	mouse_wait(MOUSE_WAIT_WRITE);
	outb(PS2_DATA, status);

    mouse_write(0xF6);
	mouse_read();
	mouse_write(0xF4);
	mouse_read();

    register_interrupt_handler(IRQ_OFFSET + 12, mouse_handler);
}