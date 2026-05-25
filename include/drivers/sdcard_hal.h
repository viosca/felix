/**
 * @file drivers/sdcard_hal.h
 * @brief The hardware-agnostic contract for SD Card HAL implementations.
 */
#ifndef FELIX_DRIVERS_SDCARD_HAL_H
#define FELIX_DRIVERS_SDCARD_HAL_H

#include <stddef.h>
#include <stdint.h>

/* Standard physical hardware states */
#define SDCARD_HAL_STATUS_READY   0x00
#define SDCARD_HAL_STATUS_NOINIT  0x01
#define SDCARD_HAL_STATUS_NODISK  0x02

/* Generic IOCTL commands (Mapped from FatFs requirements) */
#define SDCARD_HAL_CTRL_SYNC        0
#define SDCARD_HAL_GET_SECTOR_COUNT 1
#define SDCARD_HAL_GET_SECTOR_SIZE  2
#define SDCARD_HAL_GET_BLOCK_SIZE   3

int sdcard_hal_init(void);
int sdcard_hal_deinit(void);
int sdcard_hal_status(void);
int sdcard_hal_read_blocks(uint8_t *buff, uint32_t sector, uint32_t count);
int sdcard_hal_write_blocks(const uint8_t *buff, uint32_t sector, uint32_t count);
int sdcard_hal_ioctl(int cmd, void *buff);

/* Hardware IRQ control for the agnostic worker to use */
void sdcard_hal_irq_enable(void);
void sdcard_hal_irq_disable(void);
void sdcard_hal_clear_interrupts(void);
int sdcard_hal_is_inserted(void);

/* Called by the OS trap handler when the PLIC triggers an MMC interrupt */
void sdcard_hal_handle_irq(void);

#endif /* FELIX_DRIVERS_SDCARD_HAL_H */
