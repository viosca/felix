/**
 * @file felix/drivers/flx_serial_core.c
 * @brief Generic Hardware-Agnostic Serial Core Framework
 */
#include <errno.h>
#include "drivers/flx_serial_core.h"
#include "kernel/devices.h"
#include "kernel/tty.h"
#include "cpu.h"
#include "kernel/sched.h"

/* Generic State Tracking */
static void *g_hw_instances[FLX_DEV_MAX_DEVICES];
static const FlxUartOps_t *g_uart_ops[FLX_DEV_MAX_DEVICES];
static volatile bool g_tx_active[FLX_DEV_MAX_DEVICES];

/* =========================================================================
 * Hardware TTY Callbacks (Bottom Half)
 * ========================================================================= */

static void uart_hw_putc(void *ctx, char c) {
    FlxDev_t *dev = (FlxDev_t *)ctx;
    if (!dev) return;

    int id = dev->id;
    const FlxUartOps_t *ops = g_uart_ops[id];
    void *hw = g_hw_instances[id];

    if (!ops) return;

    /* APPLY BACKPRESSURE (Safe from spurious wakeups!) */
    while (ring_buffer_is_full(&dev->tx_buffer)) {
        arch_cpu_idle();
    }

    /* CRITICAL SECTION START */
    if (ops->irq_mask) ops->irq_mask(hw, true);

    ring_buffer_write(&dev->tx_buffer, (uint8_t)c);

    if (!g_tx_active[id]) {
        g_tx_active[id] = true;
        if (ops->tx_kickstart) ops->tx_kickstart(hw, &g_tx_active[id]);
    }

    /* CRITICAL SECTION END */
    if (ops->irq_mask) ops->irq_mask(hw, false);
}

/* =========================================================================
 * Core Initialization
 * ========================================================================= */

int flx_serial_init(int dev_id, void *hw_instance, const FlxUartOps_t *ops) {
    if (dev_id < 0 || dev_id >= FLX_DEV_MAX_DEVICES || !ops) return -1;

    FlxDev_t *dev = flx_dev_register(dev_id, FlxDevType_UART);
    if (!dev) return -ENOENT;

    g_hw_instances[dev_id] = hw_instance;
    g_uart_ops[dev_id] = ops;
    g_tx_active[dev_id] = false;

    ring_buffer_init(&dev->tx_buffer);
    tty_init(&dev->tty, uart_hw_putc, dev);

    /* Create the OS synchronization primitive --- */
    sched_sem_create(&dev->rx_semaphore);

    /* Hand off hardware specific initialization to the vendor driver */
    if (ops->hw_init) {
        return ops->hw_init(dev_id, hw_instance);
    }
    return 0;
}

/* =========================================================================
 * ISR Upcalls (Used by Vendor Hardware Interrupts)
 * ========================================================================= */

void flx_serial_isr_rx_byte(int dev_id, uint8_t byte) {
    FlxDev_t *dev = flx_dev_get(dev_id);
    if (dev) {
        tty_receive_char(&dev->tty, (char)byte);
        sched_sem_give_from_isr(dev->rx_semaphore);
    }
}

int flx_serial_isr_tx_pop(int dev_id, uint8_t *byte) {
    FlxDev_t *dev = flx_dev_get(dev_id);
    if (!dev || ring_buffer_is_empty(&dev->tx_buffer)) {
        g_tx_active[dev_id] = false;
        return 0; /* No data to pop */
    }
    ring_buffer_read(&dev->tx_buffer, byte);
    return 1; /* Success */
}

/* =========================================================================
 * Generic Core API Pass-through
 * ========================================================================= */

int flx_serial_read(FlxDev_t *dev, char *buf, size_t count) {
    if (!dev) return -ENODEV;
    return tty_read(&dev->tty, buf, count);
}

int flx_serial_write(FlxDev_t *dev, const char *buf, size_t count) {
    if (!dev) return -ENODEV;
    return tty_write(&dev->tty, buf, count);
}

int flx_serial_ioctl(FlxDev_t *dev, int request, void *arg) {
    if (!dev) return -ENODEV;
    return tty_ioctl(&dev->tty, request, arg);
}

/* =========================================================================
 * VFS Operations Table (Top Half)
 * ========================================================================= */

static ssize_t flx_serial_vfs_read(FlxFile_t *file, void *buf, size_t count) {
    int minor = file->node->dev_id & 0xFF;
    FlxDev_t *dev = flx_dev_get(minor);
    if (!dev) return -ENODEV;

    ssize_t ret;
    while ((ret = flx_serial_read(dev, (char *)buf, count)) == 0) {
        sched_sem_take(dev->rx_semaphore, SCHED_WAIT_FOREVER);
    }
    return ret;
}

static ssize_t flx_serial_vfs_write(FlxFile_t *file, const void *buf, size_t count) {
    int minor = file->node->dev_id & 0xFF;
    FlxDev_t *dev = flx_dev_get(minor);
    if (!dev) return -ENODEV;

    return flx_serial_write(dev, (const char *)buf, count);
}

const FlxVfsOps_t g_flx_serial_vfs_ops = {
    .read = flx_serial_vfs_read,
    .write = flx_serial_vfs_write
};
