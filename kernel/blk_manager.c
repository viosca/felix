/**
 * @file felix/kernel/blk_manager.c
 * @brief Implementation of the agnostic hotplug state machine and raw block ops.
 */
#include "kernel/blk_manager.h"
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/klog.h"
#include "drivers/sdcard_hal.h"
#include "kernel/part_mbr.h" /* Extracted MBR Parsing Subsystem */
#include "kernel/part_gpt.h" /* Extracted GPT Parsing Subsystem */
#include "kernel/part_types.h" /* 64-bit Partition Types */
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "kernel/sched.h"
#include "mpfs_hal/mss_hal.h" /* To ensure read_csr is available */

#define DEBOUNCE_TICKS_TARGET 10
#define SD_SECTOR_SIZE        512
#define MAX_PARTITIONS        8

typedef enum {
    STATE_REMOVED = 0,
    STATE_DEBOUNCING,
    STATE_PRESENT
} BlkDevState_t;

typedef struct {
    BlkDevState_t state;
    BlkDevState_t target_state;
    int debounce_counter;
    FlxPartition_t partitions[MAX_PARTITIONS]; /* Track up to 8 slots for MBR/GPT */
} BlkDevTracker_t;

static BlkDevTracker_t blk_devices[FLX_BLK_DEV_MAX];

/* =========================================================================
 * Raw SD Block Device VFS Operations
 * ========================================================================= */

static ssize_t sd_blk_read(FlxVfsFile_t *file, void *buf, size_t count) {
    if ((count % SD_SECTOR_SIZE != 0) || (file->offset % SD_SECTOR_SIZE != 0)) {
        return -EINVAL;
    }

    /* MAGIC: Extract the Minor number from the VFS dev_id */
    int minor = file->node->dev_id & 0xFF;

    uint64_t lba_offset = 0;
    uint64_t max_sectors = 0xFFFFFFFFFFFFFFFFULL; /* Assume infinite if raw drive */

    /* If Minor is 1-MAX_PARTITIONS, it's a partition! Look up the offset. */
    if (minor >= 1 && minor <= MAX_PARTITIONS) {
        lba_offset = blk_devices[FLX_BLK_DEV_SDCARD0].partitions[minor - 1].lba_start;
        max_sectors = blk_devices[FLX_BLK_DEV_SDCARD0].partitions[minor - 1].sector_count;
    }

    uint64_t sector = file->offset / SD_SECTOR_SIZE;
    uint32_t num_sectors = count / SD_SECTOR_SIZE;

    /* Prevent reading past the end of a partition */
    if (minor >= 1 && minor <= MAX_PARTITIONS && (sector + num_sectors > max_sectors)) {
        return -EINVAL;
    }

    /* Add the partition LBA offset to the physical read request! */
    /* Note: Casting to uint32_t for now until Microchip HAL is upgraded to 64-bit */
    if (sdcard_hal_read_blocks((uint8_t *)buf, (uint32_t)(lba_offset + sector), num_sectors) == 0) {
        file->offset += count;
        return count;
    }
    return -EIO;
}

static ssize_t sd_blk_write(FlxVfsFile_t *file, const void *buf, size_t count) {
    if ((count % SD_SECTOR_SIZE != 0) || (file->offset % SD_SECTOR_SIZE != 0)) {
        return -EINVAL;
    }

    int minor = file->node->dev_id & 0xFF;

    uint64_t lba_offset = 0;
    uint64_t max_sectors = 0xFFFFFFFFFFFFFFFFULL;

    if (minor >= 1 && minor <= MAX_PARTITIONS) {
        lba_offset = blk_devices[FLX_BLK_DEV_SDCARD0].partitions[minor - 1].lba_start;
        max_sectors = blk_devices[FLX_BLK_DEV_SDCARD0].partitions[minor - 1].sector_count;
    }

    uint64_t sector = file->offset / SD_SECTOR_SIZE;
    uint32_t num_sectors = count / SD_SECTOR_SIZE;

    if (minor >= 1 && minor <= MAX_PARTITIONS && (sector + num_sectors > max_sectors)) {
        return -EINVAL;
    }

    if (sdcard_hal_write_blocks((const uint8_t *)buf, (uint32_t)(lba_offset + sector), num_sectors) == 0) {
        file->offset += count;
        return count;
    }
    return -EIO;
}

static FlxVfsOps_t sd_blk_ops = {
    .read = sd_blk_read,
    .write = sd_blk_write,
};

/* =========================================================================
 * Hotplug Subsystem
 * ========================================================================= */

/* Hardware-specific read wrapper for the generic MBR/GPT parsers */
static int sd_read_sector_cb(uint64_t lba, uint8_t *buf) {
    return sdcard_hal_read_blocks(buf, (uint32_t)lba, 1);
}

static void handle_sdcard_insertion(bool auto_mount) {
    kprintf("\r\n[Kernel] SD Card insertion detected. Waking hardware...\n");

    /* 0. Wake hardware if the state was lost */
    if (sdcard_hal_status() == SDCARD_HAL_STATUS_NOINIT) {
        if (sdcard_hal_init() != 0) {
            kprintf("[Kernel] Hardware init failed. Retrying in 500ms...\n");

            /* CRITICAL FIX: The hardware failed. We cannot stay in STATE_PRESENT.
             * Roll back to DEBOUNCING to force the daemon loop to retry! */
            blk_devices[FLX_BLK_DEV_SDCARD0].state = STATE_DEBOUNCING;
            blk_devices[FLX_BLK_DEV_SDCARD0].debounce_counter = 50; /* 500ms delay */

            /* Force interrupts back on! The Microchip HAL reset likely killed them. */
            sdcard_hal_irq_enable();
            return;
        }
    }

    /* 1. Register the raw drive so the VFS knows it exists */
    if (flx_fs_dev_register_blk("sd0", 179, 0, &sd_blk_ops) != 0) {
        kprintf("[Kernel] Failed to register /dev/sd0 in VFS.\n");
        return;
    }
    kprintf("[Kernel] Block device /dev/sd0 created.\n");

    /* 2. Execute the parsers: Try GPT first, then fallback to MBR! */
    if (part_parse_gpt("sd0", 179, &sd_blk_ops, blk_devices[FLX_BLK_DEV_SDCARD0].partitions, MAX_PARTITIONS, sd_read_sector_cb) != 0) {
        kprintf("[Kernel] No GPT found, falling back to MBR...\n");
        part_parse_mbr("sd0", 179, &sd_blk_ops, blk_devices[FLX_BLK_DEV_SDCARD0].partitions, sd_read_sector_cb);
    }

    /* 3. Auto-Mount the discovered partitions */
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        /* If the parser found a valid partition type, mount it! */
        if (blk_devices[FLX_BLK_DEV_SDCARD0].partitions[i].type != 0x00) {
            char dev_path[32], mnt_path[32];
            snprintf(dev_path, sizeof(dev_path), "/dev/sd0p%d", i + 1);
            snprintf(mnt_path, sizeof(mnt_path), "/mnt/sd0p%d", i + 1);

            /* Create the dynamic mount point directory in the RAMFS */
            sys_mkdir(mnt_path, 0755);

            /* Route the mount through the VFS! */
            int res = sys_mount(dev_path, mnt_path, "fatfs", 0, NULL);
            if (res == 0) {
                kprintf("[Kernel] FAT filesystem auto-mounted at %s\n", mnt_path);
            } else {
                kprintf("[Kernel] Auto-mount failed for %s (Error %d)\n", dev_path, res);
                sys_rmdir(mnt_path);
            }
        }
    }
}

static void handle_sdcard_removal(void) {
    kprintf("\r\n[Kernel] SD Card surprise removal detected. Cleaning up...\n");

    /* 1. Unmount and destroy partition nodes */
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        if (blk_devices[FLX_BLK_DEV_SDCARD0].partitions[i].type != 0x00) {
            char mnt_path[32];
            char dev_name[16];

            snprintf(mnt_path, sizeof(mnt_path), "/mnt/sd0p%d", i + 1);
            snprintf(dev_name, sizeof(dev_name), "sd0p%d", i + 1);

            /* VFS Teardown */
            sys_umount(mnt_path);
            sys_rmdir(mnt_path);

            /* Block Manager Teardown */
            flx_fs_dev_unregister(dev_name);

            /* Clear the state tracker */
            blk_devices[FLX_BLK_DEV_SDCARD0].partitions[i].type = 0x00;
        }
    }

    /* 2. Unregister the raw drive */
    flx_fs_dev_unregister("sd0");

    /* 3. Hardware teardown */
    sdcard_hal_deinit();

    kprintf("[Kernel] Hardware secured. SD Card destroyed.\n");
}

/* =========================================================================
 * The OS Worker (Hotplug Logic)
 * ========================================================================= */

void flx_blk_manager_init(void) {

    static bool is_initialized = false;
    if (is_initialized) return;
    is_initialized = true;

    kprintf("[Kernel] Initializing Block Manager...\n");

    /* Initialize all device tracking states to REMOVED */
    for (int i = 0; i < FLX_BLK_DEV_MAX; i++) {
        blk_devices[i].state = STATE_REMOVED;
        blk_devices[i].target_state = STATE_REMOVED;
        blk_devices[i].debounce_counter = 0;
        /* Clear all partition slots */
        for(int j=0; j < MAX_PARTITIONS; j++) blk_devices[i].partitions[j].type = 0x00;
    }

    /* Kickstart the initial probe right here, so the daemon picks it up
     * as soon as it starts ticking. */
    blk_devices[FLX_BLK_DEV_SDCARD0].state = STATE_DEBOUNCING;
    blk_devices[FLX_BLK_DEV_SDCARD0].debounce_counter = 5;
}

void flx_blk_manager_push_event(FlxBlkDev_t dev, FlxBlkEvent_t event) {
    if (dev >= FLX_BLK_DEV_MAX) return;

    if (event == FLX_BLK_EVENT_STATE_CHANGE) {
        blk_devices[dev].state = STATE_DEBOUNCING;
        blk_devices[dev].debounce_counter = DEBOUNCE_TICKS_TARGET;
    }
}

void flx_blk_manager_tick(void) {
    for (int i = 0; i < FLX_BLK_DEV_MAX; i++) {
        if (blk_devices[i].state == STATE_DEBOUNCING) {

            if (blk_devices[i].debounce_counter > 0) {
                blk_devices[i].debounce_counter--;
            } else {
                int status = sdcard_hal_status();

                if (i == FLX_BLK_DEV_SDCARD0) {
                    if (status == SDCARD_HAL_STATUS_NOINIT || status == SDCARD_HAL_STATUS_READY) {
                        if (blk_devices[i].state != STATE_PRESENT) {
                            blk_devices[i].state = STATE_PRESENT;
                            handle_sdcard_insertion(true);
                        }
                    }
                    else if (status == SDCARD_HAL_STATUS_NODISK) {
                        if (blk_devices[i].state != STATE_REMOVED) {
                            blk_devices[i].state = STATE_REMOVED;
                            handle_sdcard_removal();
                        }
                    }
                    sdcard_hal_irq_enable();
                }
            }
        }
    }
}

void blk_manager_daemon_task(void *arg) {
    (void)arg;

    /* Removed the local kickstart from here to prevent race conditions.
     * The state machine is now primed safely during flx_blk_manager_init. */

    while(1) {
        flx_blk_manager_tick();
        sched_sleep_ms(10);
    }
}
