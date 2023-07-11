#ifndef DRIVERS_AHCI_H
#define DRIVERS_AHCI_H

#include <stdint.h>
#include <drivers/pci.h>

void ahci_initialize_device(pci_device_t *device);
void ahci_read(uint8_t port, uint64_t sector, uint16_t sector_count, void *dest);

#endif