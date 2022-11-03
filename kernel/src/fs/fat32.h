#ifndef FS_FAT32_H
#define FS_FAT32_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t nop[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t total_sector_count;
    uint8_t media_descriptor_type;
    uint16_t fat12_16_sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sector_count;
    uint32_t large_sector_count;
} __attribute__((packed)) bios_param_block_t;

typedef struct {
    bios_param_block_t bios_param_block;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster_num;
    uint16_t fsinfo_sector_num;
    uint16_t backup_boot_sector_num;
    uint8_t rsv0[12];
    uint8_t drive_number;
    uint8_t rsv1;
    uint8_t signature;
    uint32_t volume_serial_number;
    uint8_t volume_label[11];
    uint8_t sys_id_string[8];
} __attribute__((packed)) bios_param_block_ext_t;

typedef struct {
    uint8_t name[11];
    uint8_t attributes;
    uint8_t rsv0;
    uint8_t creation_duration;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_accessed;
    uint16_t cluster_high;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat_directory_entry_t;

typedef struct {
    uint8_t order;
    uint16_t name0[5];
    uint8_t attribute;
    uint8_t type;
    uint8_t checksum;
    uint16_t name1[6];
    uint16_t zero;
    uint16_t name2[2];
} __attribute__((packed)) fat_directory_entry_long_t;

typedef struct {
    uint8_t name[11];
    uint32_t cluster_num;
    uint32_t file_size;
    uint64_t seek_offset;
} file_descriptor_t;

typedef struct _dir_entry {
    file_descriptor_t fd;
    struct _dir_entry *last_entry;
} dir_entry_t;

void initialize_fs();
dir_entry_t *read_directory(uint32_t cluster_num);
dir_entry_t *read_root_directory();
void fread(file_descriptor_t *file_descriptor, uint64_t count, void *dest);
void fseekto(file_descriptor_t *file_descriptor, uint64_t offset);
void fseek(file_descriptor_t *file_descriptor, int64_t count);

#endif