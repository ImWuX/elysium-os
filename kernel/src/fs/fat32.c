#include "fat32.h"
#include <string.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <drivers/ahci.h>
#include <panic.h>

#define SUPPORTED_SECTOR_SIZE 512
#define FAT_ENTRY_MASK 0x0FFFFFFF
#define LAST_CLUSTER 0x0FFFFFF8
#define BAD_CLUSTER 0x0FFFFFF7

static int g_cluster_size_sectors;
static int g_cluster_size_bytes;
static int g_first_data_sector;
static uint32_t *g_fat;

inline static uint32_t next_cluster(uint32_t cluster) {
    return g_fat[cluster] & FAT_ENTRY_MASK;
}

uint32_t fat32_initialize() {
    void *page = pmm_page_request();
    ahci_read(0, 1, page);

    fat32_bios_param_block_ext_t *bpb = (fat32_bios_param_block_ext_t *) HHDM(page);
    if(bpb->bios_param_block.bytes_per_sector != SUPPORTED_SECTOR_SIZE) panic("FAT32", "Currently only 512 byte disk sectors are supported.");
    g_cluster_size_sectors = bpb->bios_param_block.sectors_per_cluster;
    g_cluster_size_bytes = bpb->bios_param_block.sectors_per_cluster * bpb->bios_param_block.bytes_per_sector;

    g_fat = heap_alloc(bpb->sectors_per_fat * bpb->bios_param_block.bytes_per_sector);
    ahci_read(bpb->bios_param_block.reserved_sector_count, bpb->sectors_per_fat, g_fat);

    g_first_data_sector = bpb->bios_param_block.reserved_sector_count + (bpb->bios_param_block.fat_count * bpb->sectors_per_fat);
    uint32_t root_cluster = bpb->root_cluster_num;

    pmm_page_release(page);
    return root_cluster;
}

uint64_t fat32_read(uint32_t cluster, void *dest, uint64_t count, uint64_t seek) {
    for(uint64_t i = 0; i < seek / g_cluster_size_bytes; i++) {
        if(cluster >= BAD_CLUSTER) return 0;
        cluster = next_cluster(cluster);
    }
    void *buffer = heap_alloc(g_cluster_size_bytes);
    uint64_t seek_offset = seek % g_cluster_size_bytes;
    count += seek_offset;
    for(uint64_t i = 0; i < count / g_cluster_size_bytes; i++) {
        if(cluster >= BAD_CLUSTER) return 0; // TODO: Return the correct amount
        ahci_read(g_first_data_sector + (cluster - 2) * g_cluster_size_sectors, g_cluster_size_sectors, buffer);
        memcpy(dest, buffer + seek_offset, g_cluster_size_bytes - seek_offset);
        dest += g_cluster_size_bytes - seek_offset;
        seek_offset = 0;
        cluster = next_cluster(cluster);
    }
    if(cluster == BAD_CLUSTER) return 0;
    ahci_read(g_first_data_sector + (cluster - 2) * g_cluster_size_sectors, g_cluster_size_sectors, buffer);
    memcpy(dest, buffer + seek_offset, count % g_cluster_size_bytes);
    heap_free(buffer);
    return 0; // TODO: Return the correct amount
}