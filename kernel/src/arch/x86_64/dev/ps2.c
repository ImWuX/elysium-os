#include "ps2.h"
#include <common/panic.h>
#include <arch/x86_64/sys/port.h>
#include <arch/x86_64/dev/ps2kb.h>

#define TIMEOUT 100000
#define ACK 0xFA

#define PS2_CMDSTS 0x64
#define PS2_DATA 0x60

#define PS2_STATUS_RDYREAD (1 << 0)
#define PS2_STATUS_RDYWRITE (1 << 1)

#define CONFIGBYTE_IRQ1 (1 << 0)
#define CONFIGBYTE_IRQ2 (1 << 1)
#define CONFIGBYTE_CLOCK1 (1 << 4)
#define CONFIGBYTE_CLOCK2 (1 << 5)
#define CONFIGBYTE_TRANSLATION (1 << 6)

#define KB_ID1 0xAB
#define KB_ID2 0x83

#define MOUSE_ID1 0x0
#define MOUSE_ID2 0x0

static bool g_port_one_operational;
static bool g_port_two_operational;
static uint8_t g_port_ids[2][2];

static bool wait_write() {
    int timeout = TIMEOUT;
    while(--timeout) if(!(x86_64_port_inb(PS2_CMDSTS) & PS2_STATUS_RDYWRITE)) return true;
    return false;
}

static bool wait_read() {
    int timeout = TIMEOUT;
    while(--timeout) if(x86_64_port_inb(PS2_CMDSTS) & PS2_STATUS_RDYREAD) return true;
    return false;
}

static void ps2_write(uint16_t port, uint8_t value) {
    if(wait_write()) return x86_64_port_outb(port, value);
    panic("PS2 Controller is not responding (WRITE)");
}

static uint8_t ps2_read(uint16_t port) {
    if(wait_read()) return x86_64_port_inb(port);
    panic("PS2 Controller is not responding (READ)");
}

bool x86_64_ps2_port_write(x86_64_ps2_port_t port, uint8_t value) {
    if(port == X86_64_PS2_PORT_TWO) {
        if(!wait_write()) return false;
        x86_64_port_outb(PS2_CMDSTS, 0xD4);
    }
    if(!wait_write()) return false;
    x86_64_port_outb(PS2_DATA, value);
    return true;
}

uint8_t x86_64_ps2_port_read(bool wait) {
    if(!wait || (wait && wait_read())) return x86_64_port_inb(PS2_DATA);
    panic("PS2 Port not responding (READ)");
}

void x86_64_ps2_port_enable(x86_64_ps2_port_t port) {
    ps2_write(PS2_CMDSTS, port == X86_64_PS2_PORT_ONE ? 0xAE : 0xA8);
}

static void port_initialize_drivers(x86_64_ps2_port_t port) {
    if(g_port_ids[port][0] == MOUSE_ID1 && g_port_ids[port][1] == MOUSE_ID2) {
        // x86_64_ps2mouse_initialize(port);
    }
    if(g_port_ids[port][0] == KB_ID1 && g_port_ids[port][1] == KB_ID2) {
        if(port == X86_64_PS2_PORT_ONE) {
            ps2_write(PS2_CMDSTS, 0x20);
            uint8_t configuration_byte = ps2_read(PS2_DATA);
            configuration_byte |= CONFIGBYTE_TRANSLATION;
            ps2_write(PS2_CMDSTS, 0x60);
            ps2_write(PS2_DATA, configuration_byte);
        }
        x86_64_ps2kb_initialize(port);
    }
}

static bool port_initialize(x86_64_ps2_port_t port) {
    uint8_t ack;
    if(!x86_64_ps2_port_write(port, 0xFF)) return false;
    if(!wait_read()) return false;
    ack = x86_64_port_inb(PS2_DATA);
    if(ack != ACK) return false;
    if(!wait_read()) return false;
    uint8_t reset = x86_64_port_inb(PS2_DATA);
    x86_64_port_inb(PS2_DATA);
    if(reset != 0xAA) return false;

    if(!x86_64_ps2_port_write(port, 0xF5)) return false;
    if(!wait_read()) return false;
    ack = x86_64_port_inb(PS2_DATA);
    if(ack != ACK) return false;

    if(!x86_64_ps2_port_write(port, 0xF2)) return false;
    if(!wait_read()) return false;
    ack = x86_64_port_inb(PS2_DATA);
    if(ack != ACK) return false;
    if(!wait_read()) return false;
    g_port_ids[port][0] = x86_64_port_inb(PS2_DATA);
    if(wait_read()) g_port_ids[port][1] = x86_64_port_inb(PS2_DATA);

    if(!x86_64_ps2_port_write(port, 0xF4)) return false;
    if(!wait_read()) return false;
    ack = x86_64_port_inb(PS2_DATA);
    if(ack != ACK) return false;
    return true;
}

void x86_64_ps2_initialize() {
    // Disable devices
    ps2_write(PS2_CMDSTS, 0xAD);
    ps2_write(PS2_CMDSTS, 0xA7);

    // Flush data buffer
    x86_64_port_inb(PS2_DATA);

    // Reset some bits in the configuration controller
    ps2_write(PS2_CMDSTS, 0x20);
    uint8_t configuration_byte = ps2_read(PS2_DATA);
    configuration_byte &= ~(CONFIGBYTE_IRQ1 | CONFIGBYTE_IRQ2 | CONFIGBYTE_TRANSLATION);
    ps2_write(PS2_CMDSTS, 0x60);
    ps2_write(PS2_DATA, configuration_byte);

    // Perform a self test
    ps2_write(PS2_CMDSTS, 0xAA);
    uint8_t self_test = ps2_read(PS2_DATA);
    if(self_test != 0x55) panic("PS2 Failed self test");
    ps2_write(PS2_CMDSTS, 0x60);
    ps2_write(PS2_DATA, configuration_byte);

    // Test if the controller is dual channel
    ps2_write(PS2_CMDSTS, 0xA8);
    ps2_write(PS2_CMDSTS, 0x20);
    configuration_byte = ps2_read(PS2_DATA);
    bool dual_channel = (configuration_byte & CONFIGBYTE_CLOCK2) == 0;
    if(dual_channel) ps2_write(PS2_CMDSTS, 0xA7);

    // Test first (and second) port
    ps2_write(PS2_CMDSTS, 0xAB);
    g_port_one_operational = ps2_read(PS2_DATA) == 0;
    g_port_two_operational = false;
    if(dual_channel) {
        ps2_write(PS2_CMDSTS, 0xA9);
        g_port_two_operational = ps2_read(PS2_DATA) == 0;
    }

    // Enable IRQs for the port(s)
    ps2_write(PS2_CMDSTS, 0x20);
    configuration_byte = ps2_read(PS2_DATA);
    if(g_port_one_operational) configuration_byte |= CONFIGBYTE_IRQ1;
    if(g_port_two_operational) configuration_byte |= CONFIGBYTE_IRQ2;
    ps2_write(PS2_CMDSTS, 0x60);
    ps2_write(PS2_DATA, configuration_byte);

    // Initialize port one
    if(g_port_one_operational) {
        ps2_write(PS2_CMDSTS, 0xAE);
        if(!port_initialize(X86_64_PS2_PORT_ONE)) g_port_one_operational = false;
        ps2_write(PS2_CMDSTS, 0xAD);
    }

    // Initialize port two
    if(g_port_two_operational) {
        ps2_write(PS2_CMDSTS, 0xA8);
        if(!port_initialize(X86_64_PS2_PORT_TWO)) g_port_two_operational = false;
        ps2_write(PS2_CMDSTS, 0xA7);
    }

    // Initialize drivers for the port(s)
    if(g_port_one_operational) port_initialize_drivers(X86_64_PS2_PORT_ONE);
    if(g_port_two_operational) port_initialize_drivers(X86_64_PS2_PORT_TWO);
}