#include "pic8259.h"

#define MASTER_CMD 0x20
#define MASTER_DATA 0x21
#define SLAVE_CMD 0xA0
#define SLAVE_DATA 0xA1

void pic8259_remap() {
    outb(MASTER_CMD, 0x11);
    outb(SLAVE_CMD, 0x11);
    outb(MASTER_DATA, 0x20);
    outb(SLAVE_DATA, 0x28);
    outb(MASTER_DATA, 0x04);
    outb(SLAVE_DATA, 0x02);
    outb(MASTER_DATA, 0x01);
    outb(SLAVE_DATA, 0x01);
    outb(MASTER_DATA, 0x0);
    outb(SLAVE_DATA, 0x0);
}

void pic8259_disable() {
    outb(MASTER_DATA, 0xFF);
    outb(SLAVE_DATA, 0xFF);
}
