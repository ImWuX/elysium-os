#include "mouse.h"
#include <panic.h>
#include <drivers/ports.h>
#include <cpu/interrupt.h>
#include <cpu/apic.h>
#include <drivers/ioapic.h>

#define MOUSE_LEFT (1 << 0)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

static uint8_t g_interrupt_vector;
static mouse_handler_t g_mouse_handler;
static uint8_t g_cycle = 0;
static uint8_t g_data[3];

static void mouse_handler(interrupt_frame_t *registers __attribute__((unused))) {
	uint8_t data = ps2_port_read(false);
	apic_eoi(g_interrupt_vector);
	g_data[g_cycle++] = data;
	if(g_cycle == 3) {
		g_cycle = 0;

		if(g_data[0] & 0x80 || g_data[0] & 0x40) return;
		int16_t rel_x = g_data[1] - ((g_data[0] << 4) & 0x100);
		int16_t rel_y = g_data[2] - ((g_data[0] << 3) & 0x100);

		bool buttons[3];
		buttons[0] = g_data[0] & MOUSE_LEFT;
		buttons[1] = g_data[0] & MOUSE_RIGHT;
		buttons[2] = g_data[0] & MOUSE_MIDDLE;

		if(g_mouse_handler) g_mouse_handler(rel_x, rel_y, buttons);
	}
}

void mouse_set_handler(mouse_handler_t mouse_handler) {
    g_mouse_handler = mouse_handler;
}

void mouse_initialize(ps2_ports_t port) {
	if(!ps2_port_write(port, 0xF6) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not set defaults");
	if(!ps2_port_write(port, 0xF3) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not request a new sample rate");
	if(!ps2_port_write(port, 0xC8) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not set a new sample rate");
	if(!ps2_port_write(port, 0xF4) || ps2_port_read(true) != 0xFA) panic("MOUSE", "Could not enable data reporting");

	int vector = interrupt_request(INTERRUPT_PRIORITY_HID, mouse_handler);
	if(vector < 0) panic("MOUSE", "Failed to acquire interrupt vector");
	g_interrupt_vector = vector;
    uint8_t irq = PS2_PORT_ONE_IRQ;
    if(port == PS2_PORT_TWO) irq = PS2_PORT_TWO_IRQ;
    ioapic_map_legacy_irq(irq, apic_id(), false, true, g_interrupt_vector);
}