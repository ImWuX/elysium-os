#ifndef DRIVERS_PCI_H
#define DRIVERS_PCI_H

#include <stdint.h>
#include <drivers/acpi.h>
#include <lib/list.h>

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t program_interface;
    uint8_t sub_class;
    uint8_t class;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
} __attribute__((packed)) pci_device_header_t;

typedef struct {
    pci_device_header_t device_header;

    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;

    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pointer;
    uint8_t rsv0;
    uint16_t rsv1;
    uint32_t rsv2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed)) pci_header0_t;

typedef struct {
    pci_device_header_t device_header;

    uint32_t bar0;
    uint32_t bar1;

    uint8_t primary_bus;
    uint8_t secondary_bus;
    uint8_t subordinate_bus;
    uint8_t secondary_latency_timer;
    uint8_t io_base_lower;
    uint8_t io_limit_lower;
    uint16_t secondary_status;
    uint16_t memory_base_lower;
    uint16_t memory_limit_lower;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint32_t prefetchable_memory_base_upper;
    uint32_t prefetchable_memory_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint8_t capability_pointer;
    uint8_t rsv0;
    uint16_t rsv1;
    uint32_t expansion_rom_base;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
} __attribute__((packed)) pci_header1_t;

typedef struct pci_device {
    uint16_t segment;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    list_t list;
} pci_device_t;

typedef struct {
    void (* initialize)(pci_device_t *device);
    uint16_t class;
    uint16_t subclass;
    uint16_t prog_if;
} pci_driver_t;

extern list_t g_pci_devices;

/**
 * @brief Enumerate PCI devices
 *
 * @param mcfg MCFG table
 */
void pci_enumerate(acpi_sdt_header_t *mcfg);

/**
 * @brief Read byte from device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @return Byte read from config
 */
uint8_t pci_config_read_byte(pci_device_t *device, uint8_t offset);

/**
 * @brief Read word from device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @return Word read from config
 */
uint16_t pci_config_read_word(pci_device_t *device, uint8_t offset);

/**
 * @brief Read doubleword from device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @return Doubleword read from config
 */
uint32_t pci_config_read_double(pci_device_t *device, uint8_t offset);

/**
 * @brief Write byte to device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @param data Byte to write to config
 */
void pci_config_write_byte(pci_device_t *device, uint8_t offset, uint8_t data);

/**
 * @brief Write word to device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @param data Word to write to config
 */
void pci_config_write_word(pci_device_t *device, uint8_t offset, uint16_t data);

/**
 * @brief Write doubleword to device config
 *
 * @param device PCI device
 * @param offset Config offset
 * @param data Doubleword to write to config
 */
void pci_config_write_double(pci_device_t *device, uint8_t offset, uint32_t data);

#endif