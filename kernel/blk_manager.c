/**
 * @file felix/kernel/blk_manager.c
 * @brief Implementation of the agnostic hotplug state machine and raw block ops.
 */
#include "kernel/blk_manager.h"
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/klog.h"
#include "drivers/sdcard_hal.h"
#include <stdio.h>
#include <errno.h>
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "kernel/sched.h"

#define DEBOUNCE_TICKS_TARGET 10
#define SD_SECTOR_SIZE        512

typedef enum {
    STATE_REMOVED = 0,
    STATE_DEBOUNCING,
    STATE_PRESENT
} BlkDevState_t;

typedef struct {
    BlkDevState_t state;
    BlkDevState_t target_state;
    int debounce_counter;
} BlkDevTracker_t;

static BlkDevTracker_t blk_devices[FLX_BLK_DEV_MAX];

/* =========================================================================
 * Raw SD Block Device VFS Operations
 * ========================================================================= */

static ssize_t sd_blk_read(FlxFile_t *file, void *buf, size_t count) {
    /* For a raw block device without a caching layer, we enforce
     * strict 512-byte sector alignment. */
    if ((count % SD_SECTOR_SIZE != 0) || (file->offset % SD_SECTOR_SIZE != 0)) {
        return -EINVAL;
    }

    uint32_t sector = file->offset / SD_SECTOR_SIZE;
    uint32_t num_sectors = count / SD_SECTOR_SIZE;

    if (sdcard_hal_read_blocks((uint8_t *)buf, sector, num_sectors) == 0) {
        file->offset += count;
        return count;
    }
    return -EIO;
}

static ssize_t sd_blk_write(FlxFile_t *file, const void *buf, size_t count) {
    if ((count % SD_SECTOR_SIZE != 0) || (file->offset % SD_SECTOR_SIZE != 0)) {
        return -EINVAL;
    }

    uint32_t sector = file->offset / SD_SECTOR_SIZE;
    uint32_t num_sectors = count / SD_SECTOR_SIZE;

    if (sdcard_hal_write_blocks((const uint8_t *)buf, sector, num_sectors) == 0) {
        file->offset += count;
        return count;
    }
    return -EIO;
}

/* VFS Operations table for the raw /dev/sd0 node */
static FlxVfsOps_t sd_blk_ops = {
    .read = sd_blk_read,
    .write = sd_blk_write,
    /* Open/Close/Ioctl can be NULL or stubbed depending on your VFS architecture */
};


/* =========================================================================
 * The OS Worker (Hotplug Logic)
 * ========================================================================= */

void flx_blk_manager_init(void) {
    for (int i = 0; i < FLX_BLK_DEV_MAX; i++) {
        blk_devices[i].state = STATE_REMOVED;
        blk_devices[i].debounce_counter = 0;
    }

    /* Boot up the hardware interrupts to watch for insertions */
    sdcard_hal_irq_enable();

    /* Safely trigger a fake event on boot just in case the card
     * was already inserted before the OS powered on! */
    flx_blk_manager_push_event(FLX_BLK_DEV_SDCARD0, FLX_BLK_EVENT_STATE_CHANGE);
}

void flx_blk_manager_push_event(FlxBlkDev_t dev, FlxBlkEvent_t event) {
    if (dev >= FLX_BLK_DEV_MAX) return;

    if (event == FLX_BLK_EVENT_STATE_CHANGE) {
        blk_devices[dev].state = STATE_DEBOUNCING;
        blk_devices[dev].debounce_counter = DEBOUNCE_TICKS_TARGET;
    }
}

static void handle_sdcard_insertion(void) {
    kprintf("[Kernel] SD Card insertion detected. Waking hardware...\n");

    if (sdcard_hal_init() == 0) {
        /* POSIX standard major number for MMC block devices is 179 */
        if (flx_fs_dev_register_blk("sd0", 179, 0, &sd_blk_ops) == 0) {
            kprintf("[Kernel] Block device /dev/sd0 created.\n");

            /* --- Auto-Mount the FAT Filesystem --- */
            /* 1. Ensure the mount point directory exists */
            sys_mkdir("/mnt", 0755);
            sys_mkdir("/mnt/sd0", 0755);

            /* 2. Attach FatFs ("0:") to the new mount point */
            if (sys_mount("0:", "/mnt/sd0", "fatfs", 0, NULL) == 0) {
                kprintf("[Kernel] FAT filesystem auto-mounted at /mnt/sd0\n");
            } else {
                kprintf("[Kernel] Auto-mount failed. Ready for manual mount.\n");
            }
        } else {
            kprintf("[Kernel] Failed to register /dev/sd0 in VFS.\n");
        }
    } else {
        kprintf("[Kernel] Failed to initialize SD Card hardware.\n");
    }
}

static void handle_sdcard_removal(void) {
//    printf("[Kernel] SD Card surprise removal detected. Cleaning up...\n");

    /* 1. Force Unmount */
    sys_umount("/mnt/sd0");

    /* 2. Unregister the raw block device */
    flx_fs_dev_unregister("sd0");

    /* 3. Hardware teardown */
    sdcard_hal_deinit();

    kprintf("[Kernel] Hardware secured. /dev/sd0 destroyed.\n");
}

void flx_blk_manager_tick(void) {
    static volatile int throttle = 0;
    //if (++throttle % 50000 != 0) return;

    for (int i = 0; i < FLX_BLK_DEV_MAX; i++) {
        if (blk_devices[i].state == STATE_DEBOUNCING) {

            if (blk_devices[i].debounce_counter > 0) {
                blk_devices[i].debounce_counter--;
            } else {
                int status = sdcard_hal_status();

                if (i == FLX_BLK_DEV_SDCARD0) {
                    if (status == SDCARD_HAL_STATUS_NOINIT || status == SDCARD_HAL_STATUS_READY) {
                        blk_devices[i].state = STATE_PRESENT;
                        handle_sdcard_insertion();
                    }
                    else if (status == SDCARD_HAL_STATUS_NODISK) {
                        blk_devices[i].state = STATE_REMOVED;
                        handle_sdcard_removal();
                    }
                    sdcard_hal_irq_enable();
                }
            }
        }
    }
}

void blk_manager_daemon_task(void *arg) {
    (void)arg;
    while(1) {
        /* Tick the block manager */
        flx_blk_manager_tick();

        /* Sleep for 10ms, then wake up and tick again */
        sched_sleep_ms(10);
    }
}
