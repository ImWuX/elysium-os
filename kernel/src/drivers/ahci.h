#ifndef DRIVERS_AHCI_H
#define DRIVERS_AHCI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NONE,
    SATA,
    SEMB,
    PM,
    SATAPI
} ahci_port_type_t;

typedef volatile struct {
    uint32_t command_list_base_address;
    uint32_t command_list_base_address_upper;
    uint32_t fis_base_address;
    uint32_t fis_base_address_upper;
    uint32_t interrupt_status;
    uint32_t interrupt_enable;
    uint32_t command_and_status;
    uint32_t rsv0;
    uint32_t task_file_data;
    uint32_t signature;
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t command_issue;
    uint32_t sata_notification;
    uint32_t fis_switch_control;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} __attribute__((packed)) ahci_hba_port_t;

typedef volatile struct {
    uint32_t host_capabilities;
    uint32_t global_host_control;
    uint32_t interrupt_status;
    uint32_t ports_implemented;
    uint32_t version;
    uint32_t ccc_control;
    uint32_t ccc_ports;
    uint32_t enclosure_mgmt_location;
    uint32_t enclosure_mgmt_control;
    uint32_t host_capabilities_ext;
    uint32_t bios_handoff_ctrlsts;

    uint8_t rsv0[0x74];
    uint8_t vendor[0x60];

    ahci_hba_port_t ports[1];
} __attribute__((packed)) ahci_hba_mem_t;

typedef struct {
    uint8_t command_fis_length: 5;
    uint8_t atapi:              1;
    uint8_t write:              1;
    uint8_t prefetchable:       1;

    uint8_t reset:                  1;
    uint8_t bist:                   1;
    uint8_t clear_busy:             1;
    uint8_t rsv0:                   1;
    uint8_t port_multiplier_port:   4;

    uint16_t prd_table_length;
    volatile uint32_t prd_byte_count;

    uint32_t command_table_descriptor_base_address;
    uint32_t command_table_descriptor_base_address_upper;

    uint32_t rsv1[4];
} __attribute__((packed)) ahci_hba_command_header_t;

typedef struct {
    uint32_t data_base_address;
    uint32_t data_base_address_upper;
    uint32_t rsv0;

    uint32_t byte_count:                22;
    uint32_t rsv1:                      9;
    uint32_t interrupt_on_completion:   1;
} __attribute__((packed)) ahci_hba_prdt_entry;

typedef struct {
    uint8_t command_fis[64];
    uint8_t atapi_command[16];
    uint8_t rsv0[48];

    ahci_hba_prdt_entry prdt_entries[1];
} __attribute__((packed)) ahci_hba_command_table_t;

typedef enum {
	FIS_TYPE_REG_H2D	= 0x27,
	FIS_TYPE_REG_D2H	= 0x34,
	FIS_TYPE_DMA_ACT	= 0x39,
	FIS_TYPE_DMA_SETUP	= 0x41,
	FIS_TYPE_DATA		= 0x46,
	FIS_TYPE_BIST		= 0x58,
	FIS_TYPE_PIO_SETUP	= 0x5F,
	FIS_TYPE_DEV_BITS	= 0xA1
} ahci_fis_type_t;

typedef struct {
    uint8_t fis_type;

    uint8_t port_multiplier_port:   4;
    uint8_t rsv0:                   3;
    uint8_t command_or_control:     1;

    uint8_t command;
    uint8_t feature_low;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;

    uint8_t count_low;
    uint8_t count_high;
    uint8_t isochronous_command_completion;
    uint8_t control;

    uint8_t rsv1[4];
} __attribute__((packed)) ahci_fis_reg_h2d_t;

void ahci_initialize_device(uint64_t bar5Address);
bool ahci_read(uint64_t first_sector, uint32_t sector_count, void *dest);

#endif