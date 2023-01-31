#include "fat32.h"
#include <memory/pmm.h>
#include <drivers/ahci.h>
#include <panic.h>

#define SECTOR_SIZE 512

static int g_cluster_size;

// void read_cluster(uint32_t cluster) {
//     void *page = pmm_page_request();
// }

void fat32_initialize() {
    void *page = pmm_page_request();
    ahci_read(0, 1, page);

    fat32_bios_param_block_ext_t *bpb = (fat32_bios_param_block_ext_t *) page;
    g_cluster_size = bpb->bios_param_block.sectors_per_cluster * SECTOR_SIZE;

    if(g_cluster_size > 0x1000) panic("FAT32", "Driver is currently unable to support cluster sizes larger than 4096 bytes");

    pmm_page_release(page);
}