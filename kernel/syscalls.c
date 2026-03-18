/* File: src/kernel/syscalls.c */
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/devices.h"
#include "drivers/serial.h"
#include <stddef.h>
#include <sys/stat.h>
#include <stdint.h>

int sys_stat(const char *pathname, struct stat *statbuf) {
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname);
    if (!node) return -1;
    return flx_vfs_stat(node, statbuf);
}

int sys_fstat(int fd, struct stat *statbuf) {
    /* Check if it is an open device */
    FlxDev_t *dev = flx_proc_fd_dev_get(fd);
    if (dev) {
        statbuf->st_mode = S_IFCHR | 0666;
        statbuf->st_size = 0;
        return 0;
    }

    /* Check if it is an open directory stream */
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (dir && dir->node) {
        return flx_vfs_stat(dir->node, statbuf);
    }

    return -1;
}

int sys_exec(const char *path, int argc, char **argv) {
    return flx_proc_exec(path, argc, argv);
}

const char* sys_getenv(const char *name) {
    return flx_proc_getenv(name);
}

/* =========================================================================
 * I/O System Calls
 * ========================================================================= */

int sys_open(const char *pathname, int flags, int mode) {
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname);
    if (!node) return -1;

    /* Currently, we only support opening hardware devices like /dev/mmuart0 */
    if (S_ISCHR(node->mode) || S_ISBLK(node->mode)) {

        /* The minor number (lower 8 bits) is our exact FlxDev_t ID! */
        dev_t rdev = (dev_t)((uintptr_t)node->data);
        int dev_id = rdev & 0xFF;

        FlxDev_t *dev = flx_dev_get(dev_id);
        if (!dev) return -1;

        /* Find a free File Descriptor (skip 0, 1, 2 as they are STDIN/OUT/ERR) */
        for (int i = 3; i < FLX_PROC_MAX_OPEN_FILES; i++) {
            if (flx_proc_fd_get(i) == NULL) {
                flx_proc_fd_bind(i, dev);
                return i; /* Return the newly allocated FD! */
            }
        }
        return -1; /* Out of file descriptors */
    }

    /* Regular files (like in FAT32) are not yet supported */
    return -1;
}

int sys_close(int fd) {
    /* If it's a device, unbind it */
    if (flx_proc_fd_get(fd) != NULL) {
        return flx_proc_fd_bind(fd, NULL);
    }

    /* If it's a directory stream, free it */
    if (flx_proc_fd_dir_get(fd) != NULL) {
        flx_proc_fd_dir_free(fd);
        return 0;
    }

    return -1; /* Invalid FD */
}

int sys_read(int fd, void *buf, size_t count) {
    FlxDev_t *dev = flx_proc_fd_dev_get(fd);
    if (!dev || !buf) return -1;

    /* Currently, all character devices route to the serial driver.
     * When we add pipes/files, we will switch on dev->type here! */
    return serial_read(dev, (char *)buf, count);
}

int sys_write(int fd, const void *buf, size_t count) {
    FlxDev_t *dev = flx_proc_fd_dev_get(fd);
    if (!dev || !buf) return -1;

    return serial_write(dev, (const char *)buf, count);
}

int sys_ioctl(int fd, int request, void *arg) {
    FlxDev_t *dev = flx_proc_fd_dev_get(fd);
    if (!dev) return -1;

    return serial_ioctl(dev, request, arg);
}
