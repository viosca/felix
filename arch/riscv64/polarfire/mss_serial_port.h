#ifndef _MSS_SERIAL_PORT_H
#define _MSS_SERIAL_PORT_H

#include "drivers/serial.h"

/* Exposes the PolarFire operations table for the kernel to use */
extern const FlxUartOps_t g_mss_uart_ops;

#endif /* _MSS_SERIAL_PORT_H */
