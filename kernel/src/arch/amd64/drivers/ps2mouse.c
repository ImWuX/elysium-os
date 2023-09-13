#include "ps2mouse.h"
#include <lib/assert.h>
#include <lib/panic.h>
#include <arch/amd64/port.h>
#include <arch/amd64/lapic.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/drivers/ioapic.h>

#define MOUSE_LEFT (1 << 0)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

static uint8_t g_interrupt_vector;
static ps2mouse_handler_t g_mouse_handler;
static uint8_t g_cycle = 0;
static uint8_t g_data[3];

static void mouse_handler([[maybe_unused]] interrupt_frame_t *registers) {
	uint8_t data = ps2_port_read(false);
	interrupt_irq_eoi(g_interrupt_vector);
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

void ps2mouse_set_handler(ps2mouse_handler_t mouse_handler) {
    g_mouse_handler = mouse_handler;
}

void ps2mouse_initialize(ps2_ports_t port) {
	ASSERTC(ps2_port_write(port, 0xF6) && ps2_port_read(true) == 0xFA, "Failed setting defaults");
	ASSERTC(ps2_port_write(port, 0xF3) && ps2_port_read(true) == 0xFA, "Failed to request sample rate");
	ASSERTC(ps2_port_write(port, 0xC8) && ps2_port_read(true) == 0xFA, "Failed to set sample rate");
	ASSERTC(ps2_port_write(port, 0xF4) && ps2_port_read(true) == 0xFA, "Failed to enable data reporting");

	int vector = interrupt_request(INTERRUPT_PRIORITY_HID, mouse_handler);
	ASSERT(vector >= 0);
	g_interrupt_vector = vector;
    uint8_t irq = PS2_PORT_ONE_IRQ;
    if(port == PS2_PORT_TWO) irq = PS2_PORT_TWO_IRQ;
    ioapic_map_legacy_irq(irq, lapic_id(), false, true, g_interrupt_vector);
}