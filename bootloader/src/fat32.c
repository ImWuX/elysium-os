#include "fat32.h"
#include <ahci.h>
#include <util.h>
#include <mm.h>
#include <memory.h>
#include <bootlog.h>

#define LAST_CLUSTER 0x0FFFFFF8
#define BAD_CLUSTER 0x0FFFFFF7
#define FREE_CLUSTER 0

static bios_param_block_ext_t *bpb;
static uint32_t *fat;
static void *cluster_buffer;

static uint32_t next_cluster(uint32_t cluster_num) {
    return (fat[cluster_num] & 0x0FFFFFFF);
}

static void read_cluster_to_buffer(uint32_t cluster_num) {
    uint32_t first_data_sector = bpb->bios_param_block.reserved_sector_count + (bpb->bios_param_block.fat_count * bpb->sectors_per_fat);
    read((cluster_num - 2) * bpb->bios_param_block.sectors_per_cluster + first_data_sector, bpb->bios_param_block.sectors_per_cluster, cluster_buffer);
}

void fread(file_descriptor_t *file_descriptor, uint64_t count, void *dest) {
    if(file_descriptor->seek_offset + count > file_descriptor->file_size) {
        count = file_descriptor->file_size - file_descriptor->seek_offset; // TODO: Prob throw an error instead of just reducing the length
    }
    uint64_t cluster_size = bpb->bios_param_block.sectors_per_cluster * 512;
    uint32_t cluster_num = file_descriptor->cluster_num;
    for(uint64_t i = 0; i < file_descriptor->seek_offset / cluster_size; i++) cluster_num = next_cluster(cluster_num);
    uint64_t seek_offset = file_descriptor->seek_offset % cluster_size;
    count += seek_offset;
    for(uint64_t i = 0; i < count / cluster_size; i++) {
        read_cluster_to_buffer(cluster_num);
        memcpy(cluster_buffer + seek_offset, dest, cluster_size - seek_offset); //TODO: This is probably super unefficient since we are copying all the data
        dest += cluster_size - seek_offset;
        seek_offset = 0;
        cluster_num = next_cluster(cluster_num);
    }
    read_cluster_to_buffer(cluster_num);
    memcpy(cluster_buffer + seek_offset, dest, count % cluster_size);
    file_descriptor->seek_offset += count;
}

void fseekto(file_descriptor_t *file_descriptor, uint64_t offset) {
    if(offset > file_descriptor->file_size) return;
    file_descriptor->seek_offset = offset;
}

void fseek(file_descriptor_t *file_descriptor, int64_t count) {
    fseekto(file_descriptor, file_descriptor->seek_offset + count);
}

dir_entry_t *read_directory(uint32_t cluster_num) {
    int current_page_entry_count = 0;
    dir_entry_t *dir_entry;
    dir_entry_t *last_dir_entry = 0;
    while(cluster_num < LAST_CLUSTER) {
        read_cluster_to_buffer(cluster_num);
        fat_directory_entry_t *entries = (fat_directory_entry_t *) cluster_buffer;
        for(int i = 0; i < bpb->bios_param_block.sectors_per_cluster * 16; i++) {
            fat_directory_entry_t entry = entries[i];
            if(entry.name[0] == 0x0) break;
            if(entry.name[0] == 0xE5) continue;
            if(entry.attributes == 0xF) continue; // Long name support

            if(current_page_entry_count == 0 || (current_page_entry_count + 1) * sizeof(dir_entry_t) >= 0x1000) {
                void* page = mm_request_page();
                map_memory(page, page);
                memset(0, page, 0x1000);
                dir_entry = (dir_entry_t *) (uint64_t) page;
                current_page_entry_count = 0;
            }

            dir_entry->fd.seek_offset = 0;
            dir_entry->fd.cluster_num = ((uint32_t) entry.cluster_high) << 16;
            dir_entry->fd.cluster_num += entry.cluster_low;
            dir_entry->fd.file_size = entry.size;
            memcpy(entry.name, dir_entry->fd.name, 11);
            dir_entry->last_entry = last_dir_entry;
            last_dir_entry = dir_entry;

            dir_entry++;
            current_page_entry_count++;
        }
        cluster_num = next_cluster(cluster_num);
    }
    return --dir_entry;
}

dir_entry_t *read_root_directory() {
    return read_directory(bpb->root_cluster_num);
}

void initialize_fs() {
    void *bpb_page = mm_request_page();
    map_memory(bpb_page, bpb_page);

    if(!read(0, 1, bpb_page)) {
        boot_log("FS could not be initialized. BPB read failed.", LOG_LEVEL_ERROR);
        return;
    }

    bpb = (bios_param_block_ext_t *) bpb_page;
    uint32_t pages_per_cluster = bpb->bios_param_block.sectors_per_cluster / 8 + (bpb->bios_param_block.sectors_per_cluster % 8 > 0 ? 1 : 0);
    cluster_buffer = mm_request_linear_pages(pages_per_cluster);

    fat = (uint32_t *) mm_request_linear_pages(bpb->sectors_per_fat / 8 + (bpb->sectors_per_fat % 8 > 0 ? 1 : 0));
    if(!read(bpb->bios_param_block.reserved_sector_count, bpb->sectors_per_fat, (void *) fat)) {
        boot_log("FS could not be initialized. FAT read failed.", LOG_LEVEL_ERROR);
        return;
    }
}

