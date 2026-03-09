/**
 * @file devices.c
 * @brief Device registration and management.
 */

#include "kernel/devices.h"
#include <string.h>

/* Static array of system devices (can be dynamic later) */
static FlxDev_t g_devices[FLX_DEV_MAX_DEVICES];

void flx_dev_init(void)
{
    memset(g_devices, 0, sizeof(g_devices));

    /* * Initialization of specific devices (like UART)
     * will happen in their respective drivers (e.g., serial_init),
     * which will fetch a slot from here and populate it.
     */
}

FlxDev_t *flx_dev_get(int id)
{
    if (id < 0 || id >= FLX_DEV_MAX_DEVICES) {
        return NULL;
    }

    /* Simple validation: check if it's actually registered */
    if (g_devices[id].type == FlxDevType_UNKNOWN) {
        return NULL;
    }

    return &g_devices[id];
}

FlxDev_t *flx_dev_register(int id, FlxDevType_t type)
{
    if (id < 0 || id >= FLX_DEV_MAX_DEVICES) return NULL;

    /* Ensure the slot isn't already claimed by another driver */
    if (g_devices[id].type != FlxDevType_UNKNOWN) {
        return NULL;
    }

    g_devices[id].id = id;
    g_devices[id].type = type;

    return &g_devices[id];
}
