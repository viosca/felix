/* @file felix/kernel/syscalls.c
 */
#define _POSIX_C_SOURCE 200809L
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/time.h"
#include "kernel/devices.h"
#include "kernel/sched.h"
#include "drivers/flx_serial_core.h"
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>  
#include <unistd.h>
#include <cpu.h>
#include <mm.h>
#include <sys/utsname.h>

/* =========================================================================
 * Internal Helpers
 * ========================================================================= */

/**
 * @brief Safely resolves a POSIX dirfd to a VFS node based on the pathname.
 * @return 0 on success, or negative error code (-EBADF, -ENOTDIR).
 */
static int resolve_dirfd_node(int dirfd, const char *pathname, FlxVfsNode_t **out_node) {
    /* POSIX Rule: If the path is absolute, dirfd is ignored entirely.
     * It doesn't even matter if dirfd is invalid (like -999). */
    if (pathname && pathname[0] == '/') {
        *out_node = NULL; /* Signals the VFS to start from the global root */
        return 0;
    }

    if (dirfd == AT_FDCWD) {
        *out_node = flx_proc_cwd_node_get();
        return 0;
    }

    FlxVfsFile_t *dir_file = flx_proc_fd_get(dirfd);
    if (!dir_file || !dir_file->in_use || !dir_file->node) {
        return -EBADF; /* Invalid File Descriptor */
    }

    if (!S_ISDIR(dir_file->node->mode)) {
        return -ENOTDIR; /* FD does not point to a directory */
    }

    *out_node = dir_file->node;
    return 0;
}

/* =========================================================================
 * Core File I/O System Calls
 * ========================================================================= */

int sys_openat(int dirfd, const char *pathname, int flags, int mode) {
    FlxVfsNode_t *base_dir = NULL;

    if (!pathname) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    /* 1. Allocate process FD first. We don't want to successfully open
     * a hardware file if the process has no FD slots left! */
    int fd = flx_proc_fd_alloc();
    if (fd < 0) return -EMFILE;

    /* 2. Defer all path routing, file creation, and hardware initialization to the VFS */
    FlxVfsNode_t *opened_node = NULL;
    err = flx_vfs_open_path(base_dir, pathname, flags, mode, &opened_node);
    if (err < 0) {
        flx_proc_fd_free(fd);
        return err;
    }

    /* 3. Bind the opened VFS node to the Process FD */
    flx_proc_fd_install(fd, opened_node);

    /* 4. Persist the flags (O_APPEND, O_RDONLY, etc.) in the FD table */
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (file) {
        file->flags = flags;
    }

    return fd;
}

int sys_open(const char *pathname, int flags, int mode) {
    return sys_openat(AT_FDCWD, pathname, flags, mode);
}

int sys_creat(const char *pathname, mode_t mode) {
    return sys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int sys_close(int fd) {
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (file && file->in_use) {
        /* VFS HOOK: Tell the driver to sync and clean up */
        if (file->node && file->node->ops && file->node->ops->close) {
            file->node->ops->close(file->node);
        }
        flx_proc_fd_free(fd);
        return 0;
    }

    return -EBADF; /* Bad file descriptor */
}

int sys_read(int fd, void *buf, size_t count) {
    if (!buf) return -EFAULT;

    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    FlxVfsNode_t *node = file->node;
    
    /* Directory safety check */
    if (S_ISDIR(node->mode)) return -EISDIR;
	
#if 0
    if (S_ISCHR(node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        ssize_t ret = serial_read(dev, (char *)buf, count);
        return (ret < 0) ? -EIO : ret;
    }

    if (S_ISREG(node->mode)) {
#else
    if (S_ISCHR(node->mode) || S_ISREG(node->mode)) {
#endif
        off_t current_offset = flx_proc_fd_offset_get(fd);
        ssize_t bytes_read = flx_vfs_read(node, buf, count, current_offset);

        if (bytes_read < 0) return -EIO;

        if (bytes_read > 0) {
            flx_proc_fd_offset_set(fd, current_offset + bytes_read);
        }
        return bytes_read;
    }

    return -EINVAL;
}

int sys_write(int fd, const void *buf, size_t count) {
    if (!buf) return -EFAULT;

    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    FlxVfsNode_t *node = file->node;
    if (S_ISDIR(node->mode)) return -EISDIR;
#if 0
    if (S_ISCHR(node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        ssize_t ret = serial_write(dev, (const char *)buf, count);
        return (ret < 0) ? -EIO : ret;
    }

    if (S_ISREG(node->mode)) {
#else
    if (S_ISCHR(node->mode) || S_ISREG(node->mode)) {
#endif
        off_t current_offset = flx_proc_fd_offset_get(fd);
        ssize_t bytes_written = flx_vfs_write(node, buf, count, current_offset);

        if (bytes_written < 0) return bytes_written;

        if (bytes_written > 0) {
            flx_proc_fd_offset_set(fd, current_offset + bytes_written);
        }
        return bytes_written;
    }

    return -EINVAL;
}

off_t sys_lseek(int fd, off_t offset, int whence) {
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return (off_t)-EBADF;

    if (S_ISCHR(file->node->mode)) return (off_t)-ESPIPE; 

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
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    if (S_ISCHR(file->node->mode) || S_ISBLK(file->node->mode)) {
        int minor = file->node->dev_id & 0xFF;
        FlxDev_t *dev = flx_dev_get(minor);
        if (!dev)
            return -ENODEV;

        int ret = flx_serial_ioctl(dev, request, arg);
        return ret; /* Let the specific error (-EINVAL, etc.) bubble up */
#if 0
        dev_t rdev = (dev_t)((uintptr_t)file->node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        int ret = flx_serial_ioctl(dev, request, arg);
        return ret; /* Let the specific error (-EINVAL, etc.) bubble up! */
#endif
    }

    return -ENOTTY;
}

int sys_isatty(int fd) {
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return 0;

    return S_ISCHR(file->node->mode) ? 1 : 0;
}

/* =========================================================================
 * File Attribute Pipelines (Stat, Size, Permissions)
 * ========================================================================= */

int sys_fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    FlxVfsNode_t *base_dir = NULL;
    if (!pathname || !statbuf) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    return flx_vfs_stat_path(base_dir, pathname, statbuf, flags);
}

int sys_stat(const char *pathname, struct stat *statbuf) {
    return sys_fstatat(AT_FDCWD, pathname, statbuf, 0);
}

int sys_lstat(const char *pathname, struct stat *statbuf) {
    /* Request symlinks not be followed */
    return sys_fstatat(AT_FDCWD, pathname, statbuf, AT_SYMLINK_NOFOLLOW);
}

int sys_fstat(int fd, struct stat *statbuf) {
    if (!statbuf) return -EFAULT;

    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    return flx_vfs_fstat(file, statbuf);
}

int sys_truncate(const char *path, off_t length) {
    FlxVfsNode_t *base_dir = NULL;
    if (!path) return -EINVAL;

    int err = resolve_dirfd_node(AT_FDCWD, path, &base_dir);
    if (err < 0) return err;

    return flx_vfs_truncate_path(base_dir, path, length);
}

int sys_ftruncate(int fd, off_t length) {
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    if (!(file->flags & (O_WRONLY | O_RDWR))) return -EACCES;

    /* Push logic completely down into the unified VFS hook */
    return flx_vfs_ftruncate(file, length);
}

int sys_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    FlxVfsNode_t *base_dir = NULL;
    if (!pathname) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, pathname, &remainder);
    if (!node) return -ENOENT;

    return flx_vfs_chmod_path(node, remainder ? remainder : "", mode);
}

int sys_chmod(const char *pathname, mode_t mode) {
    return sys_fchmodat(AT_FDCWD, pathname, mode, 0);
}

int sys_fchmod(int fd, mode_t mode) {
    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    return flx_vfs_fchmod(file, mode);
}

mode_t sys_umask(mode_t mask) {
    return flx_proc_umask_set(mask);
}

/* =========================================================================
 * Directory and File Topology System Calls
 * ========================================================================= */

int sys_mkdirat(int dirfd, const char *pathname, mode_t mode) {
    FlxVfsNode_t *base_dir = NULL;
    if (!pathname) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    return flx_vfs_mkdir(base_dir, pathname, mode);
}

int sys_mkdir(const char *path, mode_t mode) {
    return sys_mkdirat(AT_FDCWD, path, mode);
}

int sys_unlinkat(int dirfd, const char *pathname, int flags) {
    FlxVfsNode_t *base_dir = NULL;
    if (!pathname) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    if (flags & AT_REMOVEDIR) {
        return flx_vfs_rmdir(base_dir, pathname);
    }
    return flx_vfs_unlink(base_dir, pathname);
}

int sys_rmdir(const char *path) {
    return sys_unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

int sys_unlink(const char *path) {
    return sys_unlinkat(AT_FDCWD, path, 0);
}

int sys_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    FlxVfsNode_t *old_base = NULL, *new_base = NULL;
    if (!oldpath || !newpath) return -EINVAL;

    int err = resolve_dirfd_node(olddirfd, oldpath, &old_base);
    if (err < 0) return err;

    err = resolve_dirfd_node(newdirfd, newpath, &new_base);
    if (err < 0) return err;

    return flx_vfs_rename(old_base, oldpath, new_base, newpath);
}

int sys_rename(const char *oldpath, const char *newpath) {
    return sys_renameat(AT_FDCWD, oldpath, AT_FDCWD, newpath);
}

int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    FlxVfsNode_t *old_base = NULL, *new_base = NULL;
    if (!oldpath || !newpath) return -EINVAL;

    int err = resolve_dirfd_node(olddirfd, oldpath, &old_base);
    if (err < 0) return err;

    err = resolve_dirfd_node(newdirfd, newpath, &new_base);
    if (err < 0) return err;

    return flx_vfs_link(old_base, oldpath, new_base, newpath);
}

int sys_link(const char *oldpath, const char *newpath) {
    return sys_linkat(AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
}

int sys_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    FlxVfsNode_t *base_dir = NULL;
    if (!pathname) return -EINVAL;

    int err = resolve_dirfd_node(dirfd, pathname, &base_dir);
    if (err < 0) return err;

    return flx_vfs_mknod(base_dir, pathname, mode, dev);
}

int sys_mknod(const char *pathname, mode_t mode, dev_t dev) {
    return sys_mknodat(AT_FDCWD, pathname, mode, dev);
}

int sys_chdir(const char *path) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path, &remainder);
    if (!node) return -ENOENT;

    /* If crossing into a filesystem, we must open the dir node dynamically! */
    if (node->ops && node->ops->open && remainder && *remainder != '\0') {
        FlxVfsNode_t *new_node = NULL;
        int res = node->ops->open(node, remainder, O_RDONLY, &new_node);
        if (res < 0) return res;
        
        if (!S_ISDIR(new_node->mode)) {
            new_node->ops->close(new_node);
            return -ENOTDIR;
        }
        flx_proc_cwd_node_set(new_node);
        return 0;
    } else if (remainder && *remainder != '\0') {
        return -ENOENT;
    }

    if (!S_ISDIR(node->mode)) return -ENOTDIR;

    flx_proc_cwd_node_set(node);
    return 0;
}

char *sys_getcwd(char *buf, size_t size) {
    return flx_proc_getcwd(buf, size);
}

/* =========================================================================
 * Directory Stream System Calls
 * ========================================================================= */

int sys_opendir(const char *name) {
    if (!name) return -EFAULT;

    int fd = sys_open(name, O_RDONLY, 0);
    if (fd < 0) return fd;

    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !S_ISDIR(file->node->mode)) {
        sys_close(fd);
        return -ENOTDIR;
    }

    return fd;
}

int sys_readdir(int fd, struct dirent *entry) {
    if (!entry) return -EFAULT;

    FlxVfsFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;
    if (!S_ISDIR(file->node->mode)) return -ENOTDIR;

    int ret = flx_vfs_readdir(file->node, entry, file->offset);
    
    if (ret > 0) {
        file->offset++;
    }
    
    return ret;
}

int sys_closedir(int fd) {
    return sys_close(fd);
}

long sys_telldir(int fd) {
    off_t offset = flx_proc_fd_offset_get(fd);
    if (offset < 0) return -EBADF;
    return (long)offset;
}

int sys_seekdir(int fd, long loc) {
    if (loc < 0) return -EINVAL;
    if (flx_proc_fd_offset_set(fd, (off_t)loc) < 0) return -EBADF;
    return 0;
}

int sys_rewinddir(int fd) {
    return sys_seekdir(fd, 0);
}

/* =========================================================================
 * File System Mount / Unmount System Calls
 * ========================================================================= */

int sys_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
    if (!source || !target || !filesystemtype) return -EINVAL;
    return flx_vfs_mount_type(source, target, filesystemtype, mountflags, data);
}

int sys_umount(const char *target) {
    if (!target) return -EINVAL;
    return flx_vfs_unmount(target);
}

/* =========================================================================
 * Process & Time System Calls
 * ========================================================================= */

int sys_exec(const char *path, int argc, char **argv) {
    int ret = flx_proc_exec(path, argc, argv);
    return (ret < 0) ? -ENOEXEC : ret;
}

void sys_exit(int status) {
    /* (Optional) Uncomment to debug task terminations */
    // printf("\n[Kernel] Process exited with status %d.\n", status);

    /* Tear down the PCB, close FDs, free memory, and signal the RTOS */
    flx_proc_exit(status);

    /* Failsafe loop: The RTOS should kill this thread inside flx_proc_exit(),
     * but if control ever returns here, yield so we don't hog the CPU. */
    while (1) {
        sched_yield();
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

int sys_getpid(void) {
    return flx_proc_getpid();
}

int sys_kill(int pid, int sig) {
    /* TODO: Implement process signals (e.g., SIGKILL, SIGTERM).
     * For now, return "Not Implemented" to prevent silent failures. */
    return -ENOSYS;
}

int sys_getpriority(int which, id_t who) {
    /* Validation: 'which' usually corresponds to PRIO_PROCESS, PRIO_PGRP, etc. */
    /* Add logic to fetch the process/thread based on 'who' */

    /* Example: return flx_sched_get_priority(which, who); */
    return -ENOSYS; // Replace with actual implementation
}

int sys_setpriority(int which, id_t who, int prio) {
    /* Validation: Ensure 'prio' is within valid OS-defined range */

    /* Example: return flx_sched_set_priority(which, who, prio); */
    return -ENOSYS; // Replace with actual implementation
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
    (void)incr;
    /* sbrk is obsolete in Felix OS. All C-library memory allocation
     * is intercepted by the linker (--wrap=malloc) and routed directly
     * to the FreeRTOS (or other OS's) heap via our OSAL. */
    errno = ENOMEM;
    return (void *)-1;
}


/* =========================================================================
 * System Information System Calls
 * ========================================================================= */

int sys_uname(struct utsname *buf) {
    if (!buf) return -EINVAL;

    strncpy(buf->sysname, "Felix", sizeof(buf->sysname) - 1);
    strncpy(buf->nodename, "felix-mpfs", sizeof(buf->nodename) - 1);
    strncpy(buf->release, "1.0", sizeof(buf->release) - 1);
    strncpy(buf->version, "Build 2026.05", sizeof(buf->version) - 1);
    strncpy(buf->machine, "riscv64", sizeof(buf->machine) - 1);

    return 0;
}
