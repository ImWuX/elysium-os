#include "ahci.h"
#include <sys/dev.h>
#include <drivers/pci.h>
#include <stdbool.h>
#include <string.h>
#include <lib/assert.h>
#include <lib/panic.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <memory/vmm.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <lib/kprint.h>

#define PAGE_SIZE 0x1000
#define SECTOR_SIZE 512
#define SPP (PAGE_SIZE / SECTOR_SIZE)

#define PRDT_DW3_DBC(dbc) ((dbc) & 0x3FFFFF)
#define PRDT_DW3_I (1 << 31)

#define FIS_REG_H2D_FLAGS_CMD (1 << 7)
#define FIS_REG_H2D_LBA_MODE (1 << 6)

#define CAP_SAM (1 << 18)
#define CAP_NP(cap) ((cap) & 0x1F)
#define CAP_NCS(cap) (((cap) & (0x1F << 8)) >> 8)
#define CAP_S64A (1 << 31)
#define CAP2_BOH (1 << 0)

#define GHC_AE (1 << 31)
#define GHC_IE (1 << 1)

#define BOHC_BOS (1 << 0)
#define BOHC_OOS (1 << 1)
#define BOHC_BB (1 << 4)

#define PxCMD_ST (1 << 0)
#define PxCMD_CR (1 << 15)
#define PxCMD_FRE (1 << 4)
#define PxCMD_FR (1 << 14)

#define PxTFD_STS_BSY (1 << 7)
#define PxTFD_STS_DRQ (1 << 3)

#define PxSSTS_DET(ssts) ((ssts) & 0x7)
#define PxSSTS_IPM(ssts) (((ssts) & (0x7 << 8)) >> 8)

#define PxIS_TFES (1 << 30)

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define ATA_CMD_READ_DMA_EX 0x25

typedef enum {
    AHCI_PORT_TYPE_NONE,
    AHCI_PORT_TYPE_SATA,
    AHCI_PORT_TYPE_SEMB,
    AHCI_PORT_TYPE_PM,
    AHCI_PORT_TYPE_SATAPI
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
    uint32_t device_sleep;
    uint32_t rsv1[10];
    uint32_t vendor[4];
} __attribute__((packed)) ahci_port_registers_t;

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
} __attribute__((packed)) generic_host_control_t;

typedef struct {
    uint16_t flags;
    uint16_t prd_table_length;
    volatile uint32_t prd_byte_count;
    uint32_t command_table_descriptor_base_address;
    uint32_t command_table_descriptor_base_address_upper;
    uint32_t rsv0[4];
} __attribute__((packed)) ahci_command_header_t;

typedef struct {
    uint32_t data_base_address;
    uint32_t data_base_address_upper;
    uint32_t rsv0;
    uint32_t byte_count_and_flags;
} __attribute__((packed)) ahci_prdt_entry;

typedef enum {
	AHCI_FIS_TYPE_REG_H2D	= 0x27,
	AHCI_FIS_TYPE_REG_D2H	= 0x34,
	AHCI_FIS_TYPE_DMA_ACT	= 0x39,
	AHCI_FIS_TYPE_DMA_SETUP	= 0x41,
	AHCI_FIS_TYPE_DATA		= 0x46,
	AHCI_FIS_TYPE_BIST		= 0x58,
	AHCI_FIS_TYPE_PIO_SETUP	= 0x5F,
	AHCI_FIS_TYPE_DEV_BITS	= 0xA1
} ahci_fis_type_t;

typedef struct {
    uint8_t fis_type;
    uint8_t flags;
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
    uint8_t rsv0[4];
} __attribute__((packed)) ahci_fis_reg_h2d_t;

static pci_device_t *g_pci_device;
static uintptr_t g_bar5;
static uintptr_t g_command_lists[32];

static int port_find_cmd_slot(ahci_port_registers_t *port) {
    uint32_t slots = (port->sata_active | port->command_issue);
    for(int i = 0; i < 32; i++) {
        if((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

/*
    TODO: Implement a 500ms minimum wait + some timeout
    On a timeout, perform a full HBA reset on the device to recover.
*/
static void stop_port(ahci_port_registers_t *port) {
    port->command_and_status &= ~(PxCMD_ST | PxCMD_FRE);
    while(port->command_and_status & (PxCMD_FR | PxCMD_CR));
}

void ahci_read(uint8_t port, uint64_t lba, uint16_t count, void *dest) {
    if(!count) return;
    ASSERT((uintptr_t) dest & 0xFFF);
    ahci_port_registers_t *port_regs = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * port);

    int cmd_slot = -1;
    while(cmd_slot < 0) cmd_slot = port_find_cmd_slot(port_regs); // TODO: Add timeout

    ahci_command_header_t *command = (ahci_command_header_t *) HHDM(g_command_lists[port] + sizeof(ahci_command_header_t) * cmd_slot);
    memset(command, 0, sizeof(ahci_command_header_t));
    command->flags = (sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t)) & 0x1F;
    command->prd_table_length = (count + SPP - 1) / SPP;

    int command_table_page_count = (0x80 + command->prd_table_length * 4 + PAGE_SIZE - 1) / PAGE_SIZE;
    pmm_page_t *command_table = pmm_alloc_pages(command_table_page_count, PMM_GENERAL | PMM_AF_ZERO); // TODO: Consider static allocations for PRDT
    command->command_table_descriptor_base_address = (uint32_t) (uintptr_t) command_table->paddr;
    command->command_table_descriptor_base_address_upper = (uint32_t) ((uintptr_t) command_table->paddr >> 32);

    ahci_fis_reg_h2d_t *commandfis = (ahci_fis_reg_h2d_t *) HHDM(command_table->paddr);
    commandfis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    commandfis->flags = FIS_REG_H2D_FLAGS_CMD;
    commandfis->command = ATA_CMD_READ_DMA_EX;
    commandfis->lba0 = (uint8_t) lba;
    commandfis->lba1 = (uint8_t) (lba >> 8);
    commandfis->lba2 = (uint8_t) (lba >> 16);
    commandfis->lba3 = (uint8_t) (lba >> 24);
    commandfis->lba4 = (uint8_t) (lba >> 32);
    commandfis->lba5 = (uint8_t) (lba >> 40);
    commandfis->device = FIS_REG_H2D_LBA_MODE;
    commandfis->count_low = (uint8_t) count;
    commandfis->count_high = (uint8_t) (count >> 8);

    ahci_prdt_entry *prdt = (ahci_prdt_entry *) HHDM(command_table->paddr + 0x80);
    for(int i = 0; i < command->prd_table_length; i++) {
        uintptr_t address = arch_vmm_physical(&g_kernel_address_space, (uintptr_t) dest);
        uint16_t sectors = SPP;
        if(count < SPP) sectors = count;

        prdt[i].data_base_address = (uint32_t) address;
        prdt[i].data_base_address_upper = (uint32_t) (address >> 32);
        prdt[i].byte_count_and_flags = PRDT_DW3_DBC(sectors * SECTOR_SIZE - 1);
        prdt[i].byte_count_and_flags |= PRDT_DW3_I;

        dest += sectors * SECTOR_SIZE;
        count -= sectors;
    }

    int spin = 0;
    while((port_regs->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ))) {
        spin++;
        if(spin >= 1000000) {
            panic("AHCI: Port is not responding"); // TODO: This is truly dumb, we need to move away from a spinlock
            return;
        }
    }

    port_regs->command_issue = 1 << cmd_slot;
    do {
        if(port_regs->interrupt_status & PxIS_TFES) {
            panic("AHCI: Port issued an error");
            return;
        }
    } while((port_regs->command_issue & (1 << cmd_slot)) != 0);

    pmm_free(command_table);
}

void ahci_initialize_device(pci_device_t *device) {
    g_pci_device = device;
    g_bar5 = HHDM((uintptr_t) (pci_config_read_double(device, __builtin_offsetof(pci_header0_t, bar5)) & ~0b1111));
    uint16_t cmd = pci_config_read_word(device, __builtin_offsetof(pci_device_header_t, command));
    pci_config_write_word(device, __builtin_offsetof(pci_device_header_t, command), cmd | (1 << 2));

    generic_host_control_t *ghc = (generic_host_control_t *) g_bar5;
    if(!(ghc->host_capabilities & CAP_S64A)) panic("AHCI: Currently 32 bit addressing is not supported.");

    if(ghc->host_capabilities_ext & CAP2_BOH) {
        ghc->bios_handoff_ctrlsts |= BOHC_OOS;
        while(ghc->bios_handoff_ctrlsts & BOHC_BOS); // TODO: Implement a timeout
        /*
            TODO: Wait 25ms to see if the bios sets BB, if it does,
            then allow 2 seconds for the bios to finish its outstanding commands.
        */
    }

    if(!(ghc->host_capabilities & CAP_SAM)) ghc->global_host_control |= GHC_AE;

    uint8_t port_count = CAP_NP(ghc->host_capabilities) + 1;
    for(uint8_t i = 0; i < port_count; i++) {
        size_t command_list_size = sizeof(ahci_command_header_t) * 32;
        pmm_page_t *command_header = pmm_alloc_pages((command_list_size + ARCH_PAGE_SIZE - 1) / ARCH_PAGE_SIZE, PMM_GENERAL | PMM_AF_ZERO);
        g_command_lists[i] = (uintptr_t) command_header->paddr;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(ghc->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        if(port->command_and_status & (PxCMD_ST | PxCMD_CR | PxCMD_FR | PxCMD_FRE)) stop_port(port);
        port->command_list_base_address = (uint32_t) g_command_lists[i];
        port->command_list_base_address_upper = (uint32_t) (g_command_lists[i] >> 32);

        pmm_page_t *page = pmm_alloc_pages((256 + ARCH_PAGE_SIZE - 1) / ARCH_PAGE_SIZE, PMM_GENERAL | PMM_AF_ZERO);
        port->fis_base_address = (uint32_t) (uintptr_t) page->paddr;
        port->fis_base_address_upper = (uint32_t) ((uintptr_t) page->paddr >> 32);

        port->command_and_status |= PxCMD_FRE;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(ghc->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        port->sata_error = 0xFFFFFFFF;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(ghc->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        port->interrupt_status |= 0xFDC000AF;
        ghc->interrupt_status |= 1 << i;
        port->interrupt_enable = 0;
    }
    ghc->global_host_control |= GHC_IE;

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(ghc->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        if(port->task_file_data & (PxTFD_STS_BSY | PxTFD_STS_DRQ)) continue;
        if(PxSSTS_DET(port->sata_status) != 0x3) {
            if(PxSSTS_IPM(port->sata_status) <= 0x1) continue;
            panic("AHCI: Currently power modes are unsupported.");
        }

        port->command_and_status |= PxCMD_ST;
    }
}

static pci_driver_t ahci_driver = {
    .initialize = &ahci_initialize_device,
    .class = 0x1,
    .subclass = 0x6,
    .prog_if = 0x1
};

DEV_REGISTER_PCI(ahci_driver);