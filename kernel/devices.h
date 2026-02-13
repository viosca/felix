#ifndef FELIX_KERNEL_DEVICES_H
#define FELIX_KERNEL_DEVICES_H

#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "ring_buffer.h"

#define FELIX_MAX_OPEN_FILES    16

/* Device Flags */
typedef union {
    uint32_t lword;
    struct {
        /* Renamed to avoid collision with termios macros */
        uint32_t cooked_mode : 1;
        uint32_t nonblock    : 1;
        uint32_t echo_mode   : 1; /* Was ECHO, now safe */
        uint32_t reserved    : 29;
    };
} FelixDeviceFlags_t;

typedef enum {
    DEV_TYPE_UNKNOWN = 0,
    DEV_TYPE_UART,
    DEV_TYPE_SPI,
    DEV_TYPE_I2C
} device_type_t;

typedef struct {
    /* Identity */
    int fd_id;              // The File Descriptor index (0, 1, 2...)
    device_type_t type;
    FelixDeviceFlags_t flags;         // Cooked mode, etc.

    /* Hardware Pointer */
    union {
        mss_uart_instance_t *uart;
        void *generic;
    } hw;

    /* Buffers */
    ring_buffer_t rx_buf;
    ring_buffer_t tx_buf;
} felix_device_t;

extern felix_device_t g_felix_devices[FELIX_MAX_OPEN_FILES];

/* Registration API */
int felix_register_device(int fd, device_type_t type, void *hw_handle);

#endif // FELIX_KERNEL_DEVICES_H
