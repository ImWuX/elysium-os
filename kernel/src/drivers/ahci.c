#include "ahci.h"
#include <stdbool.h>
#include <string.h>
#include <panic.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>

#define PAGE_SIZE 0x1000
#define SECTOR_SIZE 512
#define SPE (PAGE_SIZE / SECTOR_SIZE)

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

void ahci_read(uint8_t port, uint64_t sector, uint16_t sector_count, void *dest) {
    ahci_port_registers_t *port_regs = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * port);

    int cmd_slot = -1;
    while(cmd_slot < 0) cmd_slot = port_find_cmd_slot(port_regs); // TODO: Add timeout

    ahci_command_header_t *command = (ahci_command_header_t *) HHDM(g_command_lists[port] + sizeof(ahci_command_header_t) * cmd_slot);
    memset(command, 0, sizeof(ahci_command_header_t));
    command->flags = (sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t)) & 0x1F;
    command->prd_table_length = (sector_count + SPE - 1) / SPE;

    int command_table_page_count = (0x80 + command->prd_table_length * 4 + PAGE_SIZE - 1) / PAGE_SIZE;
    void *command_table = pmm_page_request_dma(command_table_page_count);
    memset((void *) HHDM(command_table), 0, PAGE_SIZE * sizeof(command_table_page_count));
    command->command_table_descriptor_base_address = (uint32_t) (uintptr_t) command_table;
    command->command_table_descriptor_base_address_upper = (uint32_t) ((uintptr_t) command_table >> 32);

    ahci_prdt_entry *entries = (ahci_prdt_entry *) HHDM(command_table + 0x80);
    uint16_t sectors_left = sector_count;
    for(int i = 0; i < command->prd_table_length - 1; i++) {
        entries[i].data_base_address = (uint32_t) (uintptr_t) dest;
        entries[i].data_base_address_upper = (uint32_t) ((uintptr_t) dest >> 32);
        entries[i].byte_count_and_flags = PRDT_DW3_DBC(SPE * SECTOR_SIZE - 1);
        entries[i].byte_count_and_flags |= PRDT_DW3_I;
        dest += SPE * SECTOR_SIZE;
        sectors_left -= SPE;
    }
    entries[command->prd_table_length - 1].data_base_address = (uint32_t) (uintptr_t) dest;
    entries[command->prd_table_length - 1].data_base_address_upper = (uint32_t) ((uintptr_t) dest >> 32);
    entries[command->prd_table_length - 1].byte_count_and_flags = PRDT_DW3_DBC((sectors_left * SECTOR_SIZE) - 1);
    entries[command->prd_table_length - 1].byte_count_and_flags |= PRDT_DW3_I;

    ahci_fis_reg_h2d_t *commandfis = (ahci_fis_reg_h2d_t *) HHDM(command_table);
    commandfis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    commandfis->flags = FIS_REG_H2D_FLAGS_CMD;
    commandfis->command = ATA_CMD_READ_DMA_EX;
    commandfis->lba0 = (uint8_t) sector;
    commandfis->lba1 = (uint8_t) (sector >> 8);
    commandfis->lba2 = (uint8_t) (sector >> 16);
    commandfis->lba3 = (uint8_t) (sector >> 24);
    commandfis->lba4 = (uint8_t) (sector >> 32);
    commandfis->lba5 = (uint8_t) (sector >> 40);
    commandfis->device = FIS_REG_H2D_LBA_MODE;
    commandfis->count_low = (uint8_t) sector_count;
    commandfis->count_high = (uint8_t) (sector_count >> 8);

    int spin = 0;
    while((port_regs->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ))) {
        spin++;
        if(spin >= 1000000) {
            panic("AHCI", "Port is not responding"); // TODO: This is truly dumb, we need to move away from a spinlock
            return;
        }
    }

    port_regs->command_issue = 1 << cmd_slot;
    do {
        if(port_regs->interrupt_status & PxIS_TFES) {
            panic("AHCI", "Port issued an error");
            return;
        }
    } while((port_regs->command_issue & (1 << cmd_slot)) != 0);

    // TODO: Release DMA region, rewrite dma allocator ig
}

void ahci_initialize_device(pci_device_t *device) {
    g_pci_device = device;
    g_bar5 = HHDM((uintptr_t) (pci_config_read_double(device, __builtin_offsetof(pci_header0_t, bar5)) & ~0b1111));
    uint16_t cmd = pci_config_read_word(device, __builtin_offsetof(pci_device_header_t, command));
    pci_config_write_word(device, __builtin_offsetof(pci_device_header_t, command), cmd | (1 << 2));

    ahci_hba_registers_t *hba_regs = (ahci_hba_registers_t *) g_bar5;
    if(!(hba_regs->host_capabilities & CAP_S64A)) panic("AHCI", "Currently 32 bit addressing is not supported.");

    if(hba_regs->host_capabilities_ext & CAP2_BOH) {
        hba_regs->bios_handoff_ctrlsts |= BOHC_OOS;
        while(hba_regs->bios_handoff_ctrlsts & BOHC_BOS); // TODO: Implement a timeout
        /*
            TODO: Wait 25ms to see if the bios sets BB, if it does,
            then allow 2 seconds for the bios to finish its outstanding commands.
        */
    }

    if(!(hba_regs->host_capabilities & CAP_SAM)) hba_regs->global_host_control |= GHC_AE;

    uint8_t port_count = CAP_NP(hba_regs->host_capabilities) + 1;
    uintptr_t page = 0;
    for(uint8_t i = 0; i < port_count; i++) {
        if(i % 4 == 0) {
            page = (uintptr_t) pmm_page_request();
            memset((void *) HHDM(page), 0, PAGE_SIZE);
        }
        g_command_lists[i] = page;
        page += PAGE_SIZE / 4;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(hba_regs->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        if(port->command_and_status & (PxCMD_ST | PxCMD_CR | PxCMD_FR | PxCMD_FRE)) stop_port(port);
        port->command_list_base_address = (uint32_t) g_command_lists[i];
        port->command_list_base_address_upper = (uint32_t) (g_command_lists[i] >> 32);

        uint64_t fb = (uint64_t) pmm_page_request();
        memset((void *) HHDM(fb), 0, PAGE_SIZE);
        port->fis_base_address = (uint32_t) fb;
        port->fis_base_address_upper = (uint32_t) (fb >> 32);

        port->command_and_status |= PxCMD_FRE;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(hba_regs->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        port->sata_error = 0xFFFFFFFF;
    }

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(hba_regs->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        port->interrupt_status |= 0xFDC000AF;
        hba_regs->interrupt_status |= 1 << i;
        port->interrupt_enable = 0;
    }
    hba_regs->global_host_control |= GHC_IE;

    for(unsigned int i = 0; i < port_count; i++) {
        if(!(hba_regs->ports_implemented & (1 << i))) continue;
        ahci_port_registers_t *port = (ahci_port_registers_t *) (g_bar5 + 0x100 + sizeof(ahci_port_registers_t) * i);
        if(port->task_file_data & (PxTFD_STS_BSY | PxTFD_STS_DRQ)) continue;
        if(PxSSTS_DET(port->sata_status) != 0x3) {
            if(PxSSTS_IPM(port->sata_status) <= 0x1) continue;
            panic("AHCI", "Currently power modes are unsupported.");
        }

        port->command_and_status |= PxCMD_ST;
    }
}