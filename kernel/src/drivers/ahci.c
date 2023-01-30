#include "ahci.h"
#include <panic.h>
#include <string.h>
#include <memory/hhdm.h>
#include <memory/vmm.h>
#include <memory/pmm.h>

#define SECTOR_SIZE 512

#define	SATA_SIG_ATA	0x00000101
#define	SATA_SIG_ATAPI	0xEB140101
#define	SATA_SIG_SEMB	0xC33C0101
#define	SATA_SIG_PM     0x96690101

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000
#define HBA_PxIS_TFES   (1 << 30)

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define ATA_CMD_READ_DMA_EX 0x25

static ahci_port_type_t check_port_type(ahci_hba_port_t *port) {
    uint32_t sata_status = port->sata_status;
    uint8_t interface_power_mgmt = (sata_status >> 8) & 0b111;
    uint8_t device_detection = sata_status & 0b111;

    if(device_detection != HBA_PORT_DET_PRESENT) return NONE;
    if(interface_power_mgmt != HBA_PORT_IPM_ACTIVE) return NONE;

    switch(port->signature) {
        case SATA_SIG_ATA:
            return SATA;
        case SATA_SIG_ATAPI:
            return SATAPI;
        case SATA_SIG_PM:
            return PM;
        case SATA_SIG_SEMB:
            return SEMB;
        default:
            return NONE;
    }
}

static void start_port(ahci_hba_port_t *port) {
    while(port->command_and_status & HBA_PxCMD_CR);

    port->command_and_status |= HBA_PxCMD_FRE;
    port->command_and_status |= HBA_PxCMD_ST;
}

static void stop_port(ahci_hba_port_t *port) {
    port->command_and_status &= ~HBA_PxCMD_ST;
    port->command_and_status &= ~HBA_PxCMD_FRE;

    while(true) {
        if(port->command_and_status & HBA_PxCMD_FR) continue;
        if(port->command_and_status & HBA_PxCMD_CR) continue;
        break;
    }
}

static void configure_port(ahci_hba_port_t *port) {
    stop_port(port);

    //TODO: I was pretty lazy here, unnecessary waste of memory
    void *clb = pmm_page_alloc();
    memset((void *) HHDM(clb), 0, 1024);
    port->command_list_base_address = (uint32_t) (uintptr_t) clb;
    port->command_list_base_address_upper = (uint32_t) ((uintptr_t) clb >> 32);

    void *fb = pmm_page_alloc();
    memset((void *) HHDM(fb), 0, 256);
    port->fis_base_address = (uint32_t) (uintptr_t) fb;
    port->fis_base_address_upper = (uint32_t) ((uintptr_t) fb >> 32);

    ahci_hba_command_header_t *command_header = (ahci_hba_command_header_t *) HHDM(clb);
    for(int i = 0; i < 32; i++) {
        command_header[i].prd_table_length = 8;

        void *command_table_address = pmm_page_alloc();
        memset((void *) HHDM(command_table_address), 0, 256);
        command_header[i].command_table_descriptor_base_address = (uint32_t) (uintptr_t) command_table_address;
        command_header[i].command_table_descriptor_base_address_upper = (uint32_t) ((uintptr_t) command_table_address >> 32);
    }

    start_port(port);
}

static int find_cmd_slot(ahci_hba_port_t *port) {
    uint32_t slots = (port->sata_active | port->command_issue);
    for(int i = 0; i < 32; i++) {
        if((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

static bool read_port(ahci_hba_port_t *port, uint64_t first_sector, uint32_t sector_count, void *dest) {
    uint32_t sector_low = (uint32_t) first_sector;
    uint32_t sector_high = (uint32_t) (first_sector >> 32);

    port->interrupt_status = (uint32_t) -1;

    int slot = find_cmd_slot(port);
    if(slot == -1) return false;

    ahci_hba_command_header_t *command_header = (ahci_hba_command_header_t *) (HHDM(port->command_list_base_address) + ((uintptr_t) port->command_list_base_address_upper << 32));
    command_header += slot;
    command_header->command_fis_length = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    command_header->write = 0;
    command_header->prd_table_length = (uint16_t) ((sector_count - 1) >> 4) + 1;

    ahci_hba_command_table_t *table = (ahci_hba_command_table_t *) (HHDM(command_header->command_table_descriptor_base_address) + ((uintptr_t) command_header->command_table_descriptor_base_address_upper << 32));
    memset(table, 0, sizeof(ahci_hba_command_table_t) + (command_header->prd_table_length - 1) * sizeof(ahci_hba_prdt_entry));

    int i = 0;
    uint32_t sectors_left = sector_count;
    while(i < command_header->prd_table_length - 1) {
        table->prdt_entries[i].data_base_address = (uint32_t) (uintptr_t) dest;
        table->prdt_entries[i].data_base_address_upper = (uint32_t) ((uintptr_t) dest >> 32);
        table->prdt_entries[i].byte_count = 16 * SECTOR_SIZE - 1;
        table->prdt_entries[i].interrupt_on_completion = true;
        dest += 16 * SECTOR_SIZE;
        sectors_left -= 16;
        i++;
    }
    table->prdt_entries[i].data_base_address = (uint32_t) (uintptr_t) dest;
    table->prdt_entries[i].data_base_address_upper = (uint32_t) ((uintptr_t) dest >> 32);
    table->prdt_entries[i].byte_count = (sectors_left << 9) - 1;
    table->prdt_entries[i].interrupt_on_completion = true;

    ahci_fis_reg_h2d_t *commandfis = (ahci_fis_reg_h2d_t *) (&table->command_fis);

    commandfis->fis_type = FIS_TYPE_REG_H2D;
    commandfis->command_or_control = 1;
    commandfis->command = ATA_CMD_READ_DMA_EX;

    commandfis->lba0 = (uint8_t) sector_low;
    commandfis->lba1 = (uint8_t) (sector_low >> 8);
    commandfis->lba2 = (uint8_t) (sector_low >> 16);
    commandfis->lba3 = (uint8_t) sector_high;
    commandfis->lba4 = (uint8_t) (sector_high >> 8);
    commandfis->lba5 = (uint8_t) (sector_high >> 16);

    commandfis->device = 1 << 6; // LBA Mode
    commandfis->count_low = sector_count & 0xFF;
    commandfis->count_high = (sector_count >> 8) & 0xFF;

    int spin = 0;
    while((port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if(spin >= 1000000) {
        panic("AHCI", "Port is not responding");
        return false;
    }

    port->command_issue = 1 << slot;

    while((port->command_issue & (1 << slot)) != 0) {
        if(port->interrupt_status & HBA_PxIS_TFES) {
            panic("AHCI", "Port issued an error");
            return false;
        }
    }

    if(port->interrupt_status & HBA_PxIS_TFES) {
        panic("AHCI", "Port issued an error");
        return false;
    }

    return true;
}

static ahci_hba_port_t *g_main_drive_port;

bool ahci_read(uint64_t first_sector, uint32_t sector_count, void *dest) {
    return read_port(g_main_drive_port, first_sector, sector_count, dest);
}

void ahci_initialize_device(uintptr_t bar5_address) {
    ahci_hba_mem_t *hba_mem = (ahci_hba_mem_t *) HHDM(bar5_address);

    for(int i = 0; i < 32; i++) {
        if(!(hba_mem->ports_implemented & (1 << i))) continue;
        ahci_hba_port_t *port = &hba_mem->ports[i];
        ahci_port_type_t type = check_port_type(port);

        if(type != SATA) return;
        configure_port(port);
        g_main_drive_port = port;
    }
}