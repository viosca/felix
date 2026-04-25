/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for Felix OS                                */
/*-----------------------------------------------------------------------*/

/* 1. MUST include ff.h first to get BYTE, WORD, DWORD, LBA_t types */
#include "ff.h"

/* 2. Then include the interface contract */
#include "diskio.h"

/* 3. Include our OS-level Hardware Abstraction Layer */
#include "drivers/sdcard_hal.h"

/* Physical drive number mapped to the SD Card */
#define DEV_MMC     0


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != DEV_MMC) return STA_NOINIT;

    /* Call the HAL to initialize the hardware */
    if (sdcard_hal_init() != 0) {
        return STA_NOINIT;
    }

    return 0; /* Return 0 for success (Disk is initialized) */
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != DEV_MMC) return STA_NOINIT;

    int status = sdcard_hal_status();

    if (status == SDCARD_HAL_STATUS_NODISK) return STA_NODISK;
    if (status == SDCARD_HAL_STATUS_NOINIT) return STA_NOINIT;

    return 0; /* Return 0 for success (Disk is ready) */
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
    if (pdrv != DEV_MMC) return RES_PARERR;

    /* Pass the buffer and sector data down to the HAL */
    if (sdcard_hal_read_blocks((uint8_t *)buff, (uint32_t)sector, (uint32_t)count) == 0) {
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
    if (pdrv != DEV_MMC) return RES_PARERR;

    /* Pass the buffer and sector data down to the HAL */
    if (sdcard_hal_write_blocks((const uint8_t *)buff, (uint32_t)sector, (uint32_t)count) == 0) {
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
