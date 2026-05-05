/**
 * @file felix/include/kernel/blk_manager.h
 * @brief Event-driven Block Device Manager for hotplugging.
 */
#ifndef FELIX_INCLUDE_KERNEL_BLK_MANAGER_H
#define FELIX_INCLUDE_KERNEL_BLK_MANAGER_H

/* Generic Block Devices supported by the OS */
typedef enum {
    FLX_BLK_DEV_SDCARD0 = 0,
    FLX_BLK_DEV_MAX
} FlxBlkDev_t;

/* Generic Hardware Events */
typedef enum {
    FLX_BLK_EVENT_STATE_CHANGE = 0
} FlxBlkEvent_t;

void flx_blk_manager_init(void);

/**
 * @brief Called STRICTLY from a hardware ISR to notify the OS of a physical change.
 */
void flx_blk_manager_push_event(FlxBlkDev_t dev, FlxBlkEvent_t event);

/**
 * @brief Called from the kernel's main idle loop or a timer thread.
 * Processes debouncing and handles VFS routing safely outside of ISR context.
 */
void flx_blk_manager_tick(void);

void blk_manager_daemon_task(void *arg);

#endif /* FELIX_INCLUDE_KERNEL_BLK_MANAGER_H */
