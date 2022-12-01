#include "ahci.h"
#include <bootlog.h>
#include <memory.h>
#include <mm.h>
#include <util.h>

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

static ahci_port_type_t check_port_type(hba_port_t *port) {
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

static void start_port(hba_port_t *port) {
    while(port->command_and_status & HBA_PxCMD_CR);

    port->command_and_status |= HBA_PxCMD_FRE;
    port->command_and_status |= HBA_PxCMD_ST;
}

static void stop_port(hba_port_t *port) {
    port->command_and_status &= ~HBA_PxCMD_ST;
    port->command_and_status &= ~HBA_PxCMD_FRE;

    while(true) {
        if(port->command_and_status & HBA_PxCMD_FR) continue;
        if(port->command_and_status & HBA_PxCMD_CR) continue;
        break;
    }
}

static void configure_port(hba_port_t *port) {
    stop_port(port);

    void *clb = mm_request_page();
    map_memory(clb, clb);
    memset(0, clb, 1024);
    port->command_list_base_address = (uint32_t) (uint64_t) clb;
    port->command_list_base_address_upper = (uint32_t) ((uint64_t) clb >> 32);

    void *fb = mm_request_page();
    map_memory(fb, fb);
    memset(0, fb, 256);
    port->fis_base_address = (uint32_t) (uint64_t) fb;
    port->fis_base_address_upper = (uint32_t) ((uint64_t) fb >> 32);

    hba_command_header_t *command_header = (hba_command_header_t *) clb;
    for(int i = 0; i < 32; i++) {
        command_header[i].prd_table_length = 8;

        void *command_table_address = mm_request_page();
        map_memory(command_table_address, command_table_address);
        memset(0, command_table_address, 256);
        command_header[i].command_table_descriptor_base_address = (uint32_t) (uint64_t) command_table_address;
        command_header[i].command_table_descriptor_base_address_upper = (uint32_t) ((uint64_t) command_table_address >> 32);
    }

    start_port(port);
}

static int find_cmd_slot(hba_port_t *port) {
    uint32_t slots = (port->sata_active | port->command_issue);
    for(int i = 0; i < 32; i++) {
        if((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

static bool read_port(hba_port_t *port, uint64_t first_sector, uint32_t sector_count, void *dest) {
    uint32_t sector_low = (uint32_t) first_sector;
    uint32_t sector_high = (uint32_t) (first_sector >> 32);

    port->interrupt_status = (uint32_t) -1;

    int slot = find_cmd_slot(port);
    if(slot == -1) return false;

    hba_command_header_t *command_header = (hba_command_header_t *) ((uint64_t) port->command_list_base_address + ((uint64_t) port->command_list_base_address_upper << 32));
    command_header += slot;
    command_header->command_fis_length = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    command_header->write = 0;
    command_header->prd_table_length = (uint16_t) ((sector_count - 1) >> 4) + 1;

    hba_command_table_t *table = (hba_command_table_t *) ((uint64_t) command_header->command_table_descriptor_base_address + ((uint64_t) command_header->command_table_descriptor_base_address_upper << 32));
    memset(0, table, sizeof(hba_command_table_t) + (command_header->prd_table_length - 1) * sizeof(hba_prdt_entry));

    int i = 0;
    uint32_t sectors_left = sector_count;
    while(i < command_header->prd_table_length - 1) {
        table->prdt_entries[i].data_base_address = (uint32_t) (uint64_t) dest;
        table->prdt_entries[i].data_base_address_upper = (uint32_t) ((uint64_t) dest >> 32);
        table->prdt_entries[i].byte_count = 16 * 512 - 1;
        table->prdt_entries[i].interrupt_on_completion = true;
        dest += 16 * 512;
        sectors_left -= 16;
        i++;
    }
    table->prdt_entries[i].data_base_address = (uint32_t) (uint64_t) dest;
    table->prdt_entries[i].data_base_address_upper = (uint32_t) ((uint64_t) dest >> 32);
    table->prdt_entries[i].byte_count = (sectors_left << 9) - 1;
    table->prdt_entries[i].interrupt_on_completion = true;

    fis_reg_h2d_t *commandfis = (fis_reg_h2d_t *) (&table->command_fis);

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
        boot_log("AHCI port is not responding", LOG_LEVEL_WARNING);
        return false;
    }

    port->command_issue = 1 << slot;

    while((port->command_issue & (1 << slot)) != 0) {
        if(port->interrupt_status & HBA_PxIS_TFES) {
            boot_log("AHCI port issued an error", LOG_LEVEL_ERROR);
            return false;
        }
    }

    if(port->interrupt_status & HBA_PxIS_TFES) {
        boot_log("AHCI port issued an error", LOG_LEVEL_ERROR);
        return false;
    }

    return true;
}

static hba_port_t *main_drive_port;

bool read(uint64_t first_sector, uint32_t sector_count, void *dest) {
    return read_port(main_drive_port, first_sector, sector_count, dest);
}

void initialize_ahci_device(uint64_t bar5_address) {
    hba_mem_t *hba_mem = (hba_mem_t *) bar5_address;
    map_memory((void *) hba_mem, (void *) hba_mem);

    for(int i = 0; i < 32; i++) {
        if(!(hba_mem->ports_implemented & (1 << i))) continue;
        hba_port_t *port = &hba_mem->ports[i];
        ahci_port_type_t type = check_port_type(port);

        if(type != SATA) return;
        configure_port(port);
        main_drive_port = port;
    }
}