/**
 * @file felix/kernel/part_mbr.c
 * @brief Master Boot Record (MBR) partition parser.
 */
#include "kernel/part_mbr.h"
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "kernel/klog.h"
#include <stdio.h>

#define SD_SECTOR_SIZE 512

/* --- MBR Internal Structures --- */
typedef struct {
    uint8_t  status;
    uint8_t  chs_start[3];
    uint8_t  type;
    uint8_t  chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) mbr_part_t;

int part_parse_mbr(const char *raw_name, int major, const FlxVfsOps_t *ops, FlxPartition_t *part_array, int (*read_sector)(uint64_t lba, uint8_t *buf)) {
    uint8_t sector_buf[SD_SECTOR_SIZE];

    /* Read physical sector 0 using the provided hardware-agnostic callback */
    if (read_sector(0, sector_buf) != 0) {
        kprintf("[Kernel] Failed to read MBR sector.\n");
        return -1;
    }

    /* Validate standard boot signature */
    if (sector_buf[510] != 0x55 || sector_buf[511] != 0xAA) {
        kprintf("[Kernel] No valid MBR signature found on %s.\n", raw_name);
        return -1;
    }

    mbr_part_t *ptable = (mbr_part_t *)&sector_buf[446];

    for (int i = 0; i < 4; i++) {
        part_array[i].type = ptable[i].type;

        if (ptable[i].type != 0x00) {
            part_array[i].lba_start = ptable[i].lba_start;
            part_array[i].sector_count = ptable[i].sector_count;

            char name[16];
            snprintf(name, sizeof(name), "%sp%d", raw_name, i + 1);

            /* Register the Partition! Minor = Partition Index + 1 */
            flx_fs_dev_register_blk(name, major, i + 1, ops);

            kprintf("[Kernel] Discovered partition /dev/%s (Type: 0x%02X, Start LBA: %u)\n",
                    name, ptable[i].type, ptable[i].lba_start);
        }
    }
    return 0;
}
