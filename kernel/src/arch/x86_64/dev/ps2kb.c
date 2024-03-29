#include "ps2kb.h"
#include <common/log.h>
#include <common/assert.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/sys/lapic.h>
#include <arch/x86_64/dev/ioapic.h>

static x86_64_ps2kb_handler_t g_keyboard_handler;
static int8_t g_scancodes[128];
static uint8_t g_interrupt_vector;

static uint8_t g_layout_us[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t',   // <-- Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,      // <-- control key
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',
    0,      // Alt
    ' ',    // Space bar
    0,      // Caps lock
    0,      // 59 - F1 key ...>
    0, 0, 0, 0, 0, 0, 0, 0,
    0,      // <... F10
    0,      // 69 - Num lock
    0,      // Scroll Lock
    0,      // Home key
    0,      // Up Arrow
    0,      // Page Up
    '-',
    0,      // Left Arrow
    0,
    0,      // Right Arrow
    '+',
    0,      // 79 - End key
    0,      // Down Arrow
    0,      // Page Down
    0,      // Insert Key
    0,      // Delete Key
    0, 0, 0,
    0,      // F11 Key
    0,      // F12 Key
    0,      // All other keys are undefined
};

static void kb_interrupt_handler([[maybe_unused]] x86_64_interrupt_frame_t *registers) {
    uint8_t scancode = x86_64_ps2_port_read(false);

    if(scancode >= 0x80) {
        g_scancodes[scancode - 0x80] = 0;
        return;
    }
    g_scancodes[scancode] = 1;

    uint8_t character = g_layout_us[scancode];
    if(character >= 'a' && character <= 'z' && (g_scancodes[0x36] || g_scancodes[0x2a])) character = g_layout_us[scancode] - 32;
    if((g_scancodes[0x36] || g_scancodes[0x2a]) && character == '\'') character = '"'; // HOTFIX; we need a more proper conversion
    if(g_keyboard_handler) g_keyboard_handler(character);
}

void x86_64_ps2kb_initialize(x86_64_ps2_port_t port) {
    int vector = x86_64_interrupt_request(X86_64_INTERRUPT_PRIORITY_HID, kb_interrupt_handler);
    ASSERT(vector >= 0);
    log(LOG_LEVEL_DEBUG, "PS2", "Initialized PS2 keyboard on vector %i", vector);
    g_interrupt_vector = vector;
    uint8_t irq = X86_64_PS2_PORT_ONE_IRQ;
    if(port == X86_64_PS2_PORT_TWO) irq = X86_64_PS2_PORT_TWO_IRQ;
    x86_64_ioapic_map_legacy_irq(irq, x86_64_lapic_id(), false, true, g_interrupt_vector);
    x86_64_ps2_port_enable(port);
}

void x86_64_ps2kb_set_handler(x86_64_ps2kb_handler_t handler) {
    g_keyboard_handler = handler;
}