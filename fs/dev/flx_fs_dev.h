#ifndef FLX_FS_DEV_H
#define FLX_FS_DEV_H

/**
 * @brief Initializes the /dev filesystem and mounts it to the root VFS.
 */
int flx_fs_dev_init(void);

/**
 * @brief Registers a new device node into the /dev folder.
 * Called by the board initialization sequence to bind hardware to the VFS.
 * * @param name  The filename to appear in /dev (e.g., "mmuart1")
 * @param major The POSIX Major number identifying the driver type (e.g., 4 for TTY)
 * @param minor The POSIX Minor number identifying the hardware instance (e.g., 1)
 * @return 0 on success, -1 if the /dev system is full.
 */
int flx_fs_dev_register(const char *name, int major, int minor);

#endif /* FLX_FS_DEV_H */
