/**
 * @file felix/include/drivers/flx_serial_core.h
 * @brief Generic Hardware-Agnostic Serial Core Framework
 */
#ifndef _FLX_SERIAL_CORE_H
#define _FLX_SERIAL_CORE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "kernel/devices.h"
#include "kernel/vfs.h"

/* --- Generic UART Hardware Operations (The HAL Bridge) --- */
typedef struct {
    int  (*hw_init)(int dev_id, void *hw_instance);
    void (*tx_kickstart)(void *hw_instance, volatile bool *tx_active_flag);
    void (*irq_mask)(void *hw_instance, bool mask);
} FlxUartOps_t;

/* --- Initialization --- */
int flx_serial_init(int dev_id, void *hw_instance, const FlxUartOps_t *ops);

/* --- VFS/POSIX Pass-through API --- */
int flx_serial_read(FlxDev_t *dev, char *buf, size_t count);
int flx_serial_write(FlxDev_t *dev, const char *buf, size_t count);
int flx_serial_ioctl(FlxDev_t *dev, int request, void *arg);

/* --- Callbacks for the Vendor ISRs to use --- */
void flx_serial_isr_rx_byte(int dev_id, uint8_t byte);
int  flx_serial_isr_tx_pop(int dev_id, uint8_t *byte);
void flx_serial_isr_tx_done(int dev_id);

/* --- The Global VFS Operations Table for all Serial Ports --- */
extern const FlxVfsOps_t g_flx_serial_vfs_ops;

#endif /* _FLX_SERIAL_CORE_H */
