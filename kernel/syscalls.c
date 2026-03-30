/* File: src/kernel/syscalls.c */
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/devices.h"
#include "kernel/time.h"
#include "drivers/serial.h"
#include <stddef.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>  /* ANSI C guarantees SEEK_SET, SEEK_CUR, and SEEK_END are here! */
#include <unistd.h>
#include <cpu.h>
#include <mm.h>

int sys_stat(const char *pathname, struct stat *statbuf) {
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname);
    if (!node) return -ENOENT;

    if (flx_vfs_stat(node, statbuf) < 0) return -EIO;
    return 0;
}

int sys_fstat(int fd, struct stat *statbuf) {
    /* Check if it is an open VFS Node */
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (file && file->in_use && file->node) {
        /* Let flx_vfs_stat perfectly handle character devices, files, etc. */
        if (flx_vfs_stat(file->node, statbuf) < 0) return -EIO;
        return 0;
    }

    /* Check if it is an open directory stream */
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (dir && dir->node) {
        if (flx_vfs_stat(dir->node, statbuf) < 0) return -EIO;
        return 0;
    }

    return -EBADF; /* Bad file descriptor */
}

int sys_exec(const char *path, int argc, char **argv) {
    int ret = flx_proc_exec(path, argc, argv);
    return (ret < 0) ? -ENOEXEC : ret;
}

void sys_exit(int status) {
    /* * TODO: Eventually this will call flx_proc_exit(status) to free the PCB,
     * close open File Descriptors, and yield() to the next ready process.
     * For now, we just print the exit code and halt.
     */
    printf("\n[Kernel] Process exited with status %d.\n", status);

    /* Disable interrupts and halt the CPU in a low-power state */
    while (1) {
        arch_cpu_idle();
    }
}

/* =========================================================================
 * Environment Handling Syscalls
 * ========================================================================= */

const char* sys_getenv(const char *name) {
    return flx_proc_getenv(name);
}

int sys_setenv(const char *name, const char *value, int overwrite) {
    return flx_proc_setenv(name, value, overwrite);
}

int sys_unsetenv(const char *name) {
    return flx_proc_unsetenv(name);
}

/* =========================================================================
 * Directory & Path Syscalls
 * ========================================================================= */

int sys_chdir(const char *path) {
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path);
    if (!node) return -ENOENT;

    /* Ensure the resolved node is actually a directory */
    if (!S_ISDIR(node->mode)) return -ENOTDIR;

    flx_proc_cwd_node_set(node);
    return 0;
}

char *sys_getcwd(char *buf, size_t size) {
    /* Pass the request down to the process manager.
     * (We'll let process.c handle the buffer length checks!)
     */
    return flx_proc_getcwd(buf, size);
}


/* =========================================================================
 * Process & Time Syscalls
 * ========================================================================= */

int sys_getpid(void) {
    return flx_proc_getpid();
}

int sys_kill(int pid, int sig) {
    /* TODO: Implement process signals (e.g., SIGKILL, SIGTERM).
     * For now, return "Not Implemented" to prevent silent failures. */
    return -ENOSYS;
}

int sys_gettimeofday(struct timeval *tv, void *tz) {
    if (!tv) return -EINVAL;

    /* Route the request down into the core Time subsystem */
    return flx_time_get_timeofday(tv);
}

int sys_settimeofday(const struct timeval *tv, const void *tz) {
    if (!tv) return -EINVAL;

    /* Route the request down into the core Time subsystem */
    return flx_time_set_timeofday(tv);
}

int sys_usleep(uint64_t usec) {
    /* Route the sleep request down into the core Time subsystem */
    return flx_time_usleep(usec);
}


/* =========================================================================
 * Memory Management System Calls
 * ========================================================================= */

void *sys_sbrk(ptrdiff_t incr) {
    /* If we had Virtual Memory or process quotas, we would check them here
     * before asking the architecture to move the physical heap boundary */
    return arch_sbrk(incr);
}

/* =========================================================================
 * I/O System Calls
 * ========================================================================= */

int sys_open(const char *pathname, int flags, int mode) {
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname);
    if (!node) return -ENOENT; /* No such file or directory */

    /* Allocate the lowest available File Descriptor integer */
    int fd = flx_proc_fd_alloc();
    if (fd < 0) return -EMFILE; /* Process hit FD limit */

    /* Install the VFS Node into the reserved FD slot */
    if (flx_proc_fd_install(fd, node) < 0) {
        flx_proc_fd_free(fd);
        return -EFAULT;
    }

    return fd;
}

int sys_close(int fd) {
    /* If it's a regular file or device, free it */
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (file && file->in_use) {
        flx_proc_fd_free(fd);
        return 0;
    }

    /* If it's a directory stream, free it */
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (dir && dir->in_use) {
        flx_proc_fd_dir_free(fd);
        return 0;
    }

    return -EBADF; /* Bad file descriptor */
}

int sys_read(int fd, void *buf, size_t count) {
    if (!buf) return -EFAULT; /* Bad address */

    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    FlxVfsNode_t *node = file->node;

    if (S_ISCHR(node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        ssize_t ret = serial_read(dev, (char *)buf, count);
        return (ret < 0) ? -EIO : ret;
    }

    if (S_ISREG(node->mode)) {
        if (!node->ops || !node->ops->read) return -ENOTSUP;

        ssize_t ret = node->ops->read(node, buf, count, file->offset);
        if (ret > 0) {
            file->offset += ret;
        }
        return ret;
    }

    return -EINVAL;
}

int sys_write(int fd, const void *buf, size_t count) {
    if (!buf) return -EFAULT;

    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    FlxVfsNode_t *node = file->node;

    if (S_ISCHR(node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        ssize_t ret = serial_write(dev, (const char *)buf, count);
        return (ret < 0) ? -EIO : ret;
    }

    if (S_ISREG(node->mode)) {
        if (!node->ops || !node->ops->write) return -ENOTSUP;

        ssize_t ret = node->ops->write(node, buf, count, file->offset);
        if (ret > 0) {
            file->offset += ret;
        }
        return ret;
    }

    return -EINVAL;
}

int sys_isatty(int fd) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return 0;

    return S_ISCHR(file->node->mode) ? 1 : 0;
}

off_t sys_lseek(int fd, off_t offset, int whence) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return (off_t)-EBADF;

    if (S_ISCHR(file->node->mode)) {
        return (off_t)-ESPIPE; /* Can't seek a character device */
    }

    off_t size = 0;
    struct stat st;
    if (flx_vfs_stat(file->node, &st) == 0) {
        size = st.st_size;
    }

    off_t new_offset = file->offset;
    if (whence == SEEK_SET) new_offset = offset;
    else if (whence == SEEK_CUR) new_offset = file->offset + offset;
    else if (whence == SEEK_END) new_offset = size + offset;
    else return (off_t)-EINVAL;

    if (new_offset < 0) return (off_t)-EINVAL;

    file->offset = new_offset;
    return new_offset;
}

int sys_ioctl(int fd, int request, void *arg) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    if (S_ISCHR(file->node->mode) || S_ISBLK(file->node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)file->node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        int ret = serial_ioctl(dev, request, arg);
        return (ret < 0) ? -ENOTTY : ret; /* Inappropriate ioctl for device */
    }

    return -ENOTTY;
}
