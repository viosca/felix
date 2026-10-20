/**
 * @file felix/kernel/part_gpt.c
 * @brief GUID Partition Table (GPT) parser.
 */
#include "kernel/part_gpt.h"
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "kernel/klog.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SD_SECTOR_SIZE 512

/* --- GPT Internal Structures --- */
struct gpt_header {
    uint8_t  signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t size_of_partition_entry;
    uint32_t partition_entry_array_crc32;
} __attribute__((packed));

struct gpt_entry {
    uint8_t  partition_type_guid[16];
    uint8_t  unique_partition_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t partition_name[36]; /* UTF-16LE string */
} __attribute__((packed));

int part_parse_gpt(const char *raw_name, int major, const FlxVfsOps_t *ops,
                   FlxPartition_t *part_array, int max_partitions,
                   int (*read_sector)(uint64_t lba, uint8_t *buf))
{
    uint8_t sector_buf[SD_SECTOR_SIZE] __attribute__((aligned(8)));

    /* 1. Read LBA 1 (Primary GPT Header) */
    if (read_sector(1, sector_buf) != 0) {
        kprintf("[Kernel] Failed to read GPT header (LBA 1).\n");
        return -1;
    }

    struct gpt_header *header = (struct gpt_header *)sector_buf;

    /* 2. Validate "EFI PART" signature */
    if (memcmp(header->signature, "EFI PART", 8) != 0) {
        /* Not an error, just means this disk isn't formatted as GPT */
        return -1;
    }

    kprintf("[Kernel] Valid GPT signature found on %s.\n", raw_name);

    /* 3. Cache header values BEFORE sector_buf is overwritten with partition entries */
    uint64_t current_lba = header->partition_entry_lba;
    uint32_t num_entries = header->num_partition_entries;
    uint32_t entry_size = header->size_of_partition_entry;

    /* Sanity checks on header values */
    if (entry_size < sizeof(struct gpt_entry) || entry_size > SD_SECTOR_SIZE) {
        kprintf("[Kernel] Invalid GPT entry size: %u\n", (unsigned int)entry_size);
        return -1;
    }
    if (num_entries == 0 || num_entries > 1024) {
        kprintf("[Kernel] Invalid GPT entry count: %u\n", (unsigned int)num_entries);
        return -1;
    }

    /* Calculate how many entries fit in a 512-byte sector */
    int entries_per_sector = SD_SECTOR_SIZE / entry_size;
    uint32_t entries_read = 0;
    int valid_parts_found = 0;

    while (entries_read < num_entries && valid_parts_found < max_partitions) {

        if (read_sector(current_lba, sector_buf) != 0) {
            kprintf("[Kernel] I/O error reading GPT entries at LBA %lu.\n", (unsigned long)current_lba);
            return -1;
        }

        for (int i = 0; i < entries_per_sector; i++) {
            if (entries_read >= num_entries || valid_parts_found >= max_partitions) {
                break;
            }

            struct gpt_entry *entry = (struct gpt_entry *)&sector_buf[i * entry_size];

            /* Check if unused (Partition Type GUID is all zeros) */
            int is_unused = 1;
            for (int j = 0; j < 16; j++) {
                if (entry->partition_type_guid[j] != 0) {
                    is_unused = 0;
                    break;
                }
            }

            if (!is_unused) {
                int part_num = entries_read + 1;
                if (entries_read < (uint32_t)max_partitions) {
                    /* Check for Hart Software Services boot payload partition GUID:
                     * {21686148-6449-6e6f-744e-656564454649} ("Hah!IdontNeedEFI") */
                    static const uint8_t guid_hss_boot[16] = {
                        0x48, 0x61, 0x68, 0x21, 0x49, 0x64, 0x6F, 0x6E,
                        0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49
                    };
                    bool is_hss_boot = (memcmp(entry->partition_type_guid, guid_hss_boot, 16) == 0);

                    part_array[entries_read].type = is_hss_boot ? FLX_PART_TYPE_HSS_BOOT : FLX_PART_TYPE_GPT;
                    part_array[entries_read].lba_start = entry->starting_lba;
                    part_array[entries_read].sector_count = (entry->ending_lba - entry->starting_lba) + 1;

                    char name[16];
                    snprintf(name, sizeof(name), "%sp%d", raw_name, part_num);

                    /* Register the Partition! Minor = part_num */
                    flx_fs_dev_register_blk(name, major, part_num, ops);

                    if (is_hss_boot) {
                        kprintf("[Kernel] Discovered GPT partition /dev/%s [HSS Boot Payload] (Start LBA: %lu, Sectors: %lu)\n",
                                name, (unsigned long)entry->starting_lba, (unsigned long)part_array[entries_read].sector_count);
                    } else {
                        kprintf("[Kernel] Discovered GPT partition /dev/%s (Start LBA: %lu, Sectors: %lu)\n",
                                name, (unsigned long)entry->starting_lba, (unsigned long)part_array[entries_read].sector_count);
                    }
                }
                valid_parts_found++;
            }
            entries_read++;
        }
        current_lba++; /* Move to the next sector of the array */
    }

    return (valid_parts_found > 0) ? 0 : -1;
}
