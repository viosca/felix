/**
 * @file uart_userdata.h
 * @brief Bridge between vendor MSS UART userdata and Felix OS device wrapper.
 */

#ifndef _UART_USERDATA_H
#define _UART_USERDATA_H

#include <stddef.h>
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "kernel/devices.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum to identify what type of pointer is inside the generic container.
 */
typedef enum {
    UARTUserDataType_UNKNOWN = 0,
    UARTUserDataType_DEVICE  = 1, /* Points to a FlxDev_t */
} UARTUserDataType_t;

/**
 * @brief Generic wrapper assigned to vendor HAL 'user_data' pointers.
 */
typedef struct {
    UARTUserDataType_t type;
    union {
        FlxDev_t *device;
        void *generic;
    };
} UARTUserData_t;

/**
 * @brief Injects the generic Felix userdata into a vendor UART instance.
 * @param uart_ptr Raw pointer to mss_uart_instance_t
 * @param userdata Typed pointer to the Felix userdata to attach
 */
static inline void uart_set_userdata(mss_uart_instance_t *uart, UARTUserData_t *userdata) {
    if (uart) {
        uart->user_data = (void *)userdata;
    }
}


/**
 * @brief Safely extracts the generic Felix wrapper from a vendor UART instance.
 * @param uart_ptr Raw pointer to mss_uart_instance_t
 * @return Typed pointer to FlxHalUserData_t, or NULL.
 */
static inline UARTUserData_t* uart_get_userdata(mss_uart_instance_t *uart) {
    if (!uart)
        return NULL;
    return uart->user_data;

}

/**
 * @brief Safely extracts the generic Felix wrapper from a vendor UART instance.
 * @param uart_ptr Raw pointer to mss_uart_instance_t
 * @return Typed pointer to FlxHalUserData_t, or NULL.
 */
static inline FlxDev_t* uart_get_dev(mss_uart_instance_t *uart) {
    if (!uart)
        return NULL;

    UARTUserData_t *user_data = uart_get_userdata(uart);
    if (!user_data || user_data->type != UARTUserDataType_DEVICE)
        return NULL;

    return user_data->device;
}


#ifdef __cplusplus
}
#endif

#endif /* _UART_USERDATA_H */
