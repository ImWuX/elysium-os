#include "mouse.h"
#include <panic.h>
#include <drivers/ports.h>
#include <cpu/irq.h>

#define MOUSE_LEFT (1 << 0)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

static uint8_t g_cycle = 0;
static uint8_t g_data[3];

static mouse_handler_t g_handler;

static void mouse_handler(irq_cpu_register_t registers __attribute__((unused))) {
	uint8_t data = ps2_port_read(false);
	g_data[g_cycle++] = data;
	if(g_cycle == 3) {
		g_cycle = 0;

		if(g_data[0] & 0x80 || g_data[0] & 0x40) return;
		int32_t rel_x = g_data[1] - ((g_data[0] << 4) & 0x100);
		int32_t rel_y = g_data[2] - ((g_data[0] << 3) & 0x100);

		bool buttons[3];
		buttons[0] = g_data[0] & MOUSE_LEFT;
		buttons[1] = g_data[0] & MOUSE_RIGHT;
		buttons[2] = g_data[0] & MOUSE_MIDDLE;

		if(g_handler) g_handler(rel_x, rel_y, buttons);
	}
}

void mouse_set_handler(mouse_handler_t mouse_handler) {
    g_handler = mouse_handler;
}

void mouse_initialize(ps2_ports_t port) {
	if(!ps2_port_write(port, 0xF6) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not set defaults");
	if(!ps2_port_write(port, 0xF3) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not request a new sample rate");
	if(!ps2_port_write(port, 0xC8) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not set a new sample rate");
	if(!ps2_port_write(port, 0xF4) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not enable data reporting");

    irq_register_handler(32 + 6 + port, mouse_handler);
}