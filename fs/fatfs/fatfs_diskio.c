/**
 * @file felix/fs/fatfs/fatfs_diskio.c
 */
/*----------------------------------------------------------------------*/
/* Low level disk I/O module for Felix OS                                */
/*-----------------------------------------------------------------------*/

/* 1. MUST include ff.h first to get BYTE, WORD, DWORD, LBA_t types */
#define DIR FF_DIR
/* 1. MUST include ff.h first to get BYTE, WORD, DWORD, LBA_t types */
#include "ff.h"
#undef DIR

/* 2. Then include the interface contract */
#include "diskio.h"

/* 3. Include our OS-level Hardware Abstraction Layer */
#include "drivers/sdcard_hal.h"
#include "kernel/vfs.h"

/* Physical drive number mapped to the SD Card */
#define DEV_MMC     0

extern FlxVfsNode_t* fatfs_get_blk_node(BYTE pdrv);

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    /* The Block Manager handles physical initialization.
     * If the node exists in our translation table, it's ready to go. */
    if (fatfs_get_blk_node(pdrv) != NULL) {
        return 0;
    }

    return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    /* As long as the VFS node exists, the disk is "ready" from FatFs's perspective */
    if (fatfs_get_blk_node(pdrv) != NULL) {
        return 0;
    }

    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    FlxVfsNode_t *blk_node = fatfs_get_blk_node(pdrv);
    if (!blk_node) return RES_NOTRDY;

    /* Route the read through the VFS!
     * The Block Manager will automatically add the partition LBA offset. */
    off_t byte_offset = (off_t)sector * 512;
    size_t byte_count = (size_t)count * 512;

    ssize_t bytes_read = flx_vfs_read(blk_node, buff, byte_count, byte_offset);

    if (bytes_read == byte_count) {
        return RES_OK;
    }

    return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    FlxVfsNode_t *blk_node = fatfs_get_blk_node(pdrv);
    if (!blk_node) return RES_NOTRDY;

    /* Route the write through the VFS! */
    off_t byte_offset = (off_t)sector * 512;
    size_t byte_count = (size_t)count * 512;

    ssize_t bytes_written = flx_vfs_write(blk_node, buff, byte_count, byte_offset);

    if (bytes_written == byte_count) {
        return RES_OK;
    }

    return RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    if (pdrv != DEV_MMC) return RES_PARERR;

    int hal_cmd;

    /* Map FatFs specific commands to generic OS HAL commands */
    switch (cmd) {
        case CTRL_SYNC:
            hal_cmd = SDCARD_HAL_CTRL_SYNC;
            break;
        case GET_SECTOR_COUNT:
            hal_cmd = SDCARD_HAL_GET_SECTOR_COUNT;
            break;
        case GET_SECTOR_SIZE:
            hal_cmd = SDCARD_HAL_GET_SECTOR_SIZE;
            break;
        case GET_BLOCK_SIZE:
            hal_cmd = SDCARD_HAL_GET_BLOCK_SIZE;
            break;
        default:
            return RES_PARERR;
    }

    /* Send the mapped command to the HAL */
    if (sdcard_hal_ioctl(hal_cmd, buff) == 0) {
        return RES_OK;
    }

    return RES_ERROR;
}
