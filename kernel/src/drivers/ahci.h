#pragma once
#include <stdint.h>
#include <drivers/pci.h>

/**
 * @brief Initialize AHCI driver for a device
 *
 * @param device PCI device
 */
void ahci_initialize_device(pci_device_t *device);

/**
 * @brief Read sectors from disk
 * 
 * @param port Port
 * @param sector LBA
 * @param sector_count Sector count
 * @param dest Destination
 */
void ahci_read(uint8_t port, uint64_t sector, uint16_t sector_count, void *dest);