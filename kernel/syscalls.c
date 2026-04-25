/* File: src/kernel/syscalls.c */
#include "kernel/syscalls.h"
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/time.h"
#include "kernel/devices.h"
#include "drivers/serial.h"
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

int sys_stat(const char *pathname, struct stat *statbuf) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname, &remainder);
    if (!node) return -ENOENT;

    /* If it's a dynamic filesystem (like FatFs), pass the remainder path to the driver! */
    if (node->ops && node->ops->getattr) {
        return node->ops->getattr(node, remainder, statbuf);
    } else if (remainder && *remainder != '\0') {
        return -ENOENT;
    }

    if (flx_vfs_stat(node, statbuf) < 0) return -EIO;
    return 0;
}

int sys_fstat(int fd, struct stat *statbuf) {
    /* We just check the one FD table */
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;
    
    if (flx_vfs_stat(file->node, statbuf) < 0) return -EIO;
    return 0;
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

    /* Ensure the resolved static node is actually a directory */
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
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname, &remainder);
    
    if (!node) return -ENOENT;

    /* REFACTORED: If there is an unresolved remainder and the target node does NOT have custom 
     * open operations (like FatFs), we try to create it natively in the VFS/RAMFS. */
    if (remainder && *remainder != '\0' && !(node->ops && node->ops->open)) {
        if (flags & O_CREAT) {
            int ret = flx_vfs_create(pathname, mode ? mode : 0666);
            if (ret < 0 && ret != -EEXIST) return ret;
            
            /* Re-resolve the path now that the file exists in the VFS */
            remainder = NULL;
            node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname, &remainder);
            if (!node || (remainder && *remainder != '\0')) return -ENOENT;
        } else {
            return -ENOENT; /* File does not exist and O_CREAT wasn't passed */
        }
    }

    /* VFS HOOK: Let the driver initialize resources (like allocating FatFs FIL) */
    if (node->ops && node->ops->open) {
        FlxVfsNode_t *new_node = NULL;
        int res = node->ops->open(node, remainder, flags, &new_node);
        if (res < 0) return res;
        
        /* Overwrite the root node with the newly generated file node */
        node = new_node; 
    } else if (remainder && *remainder != '\0') {
        /* Path didn't fully resolve, and this isn't a dynamic filesystem */
        if (flags & O_CREAT) return -ENOSYS;
        return -ENOENT;
    }

    /* Allocate the lowest available File Descriptor integer */
    int fd = flx_proc_fd_alloc();
    if (fd < 0) {
        if (node->ops && node->ops->close) node->ops->close(node);
        return -EMFILE; /* Process hit FD limit */
    }

    /* Install the VFS Node into the reserved FD slot */
    if (flx_proc_fd_install(fd, node) < 0) {
        if (node->ops && node->ops->close) node->ops->close(node);
        flx_proc_fd_free(fd);
        return -EFAULT;
    }

    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (file) {
        file->flags = flags;
    }

    return fd;
}

int sys_creat(const char *pathname, mode_t mode) {
    return sys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int sys_close(int fd) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
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

    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    FlxVfsNode_t *node = file->node;
    
    /* Directory safety check remains identical */
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

    FlxOpenFile_t *file = flx_proc_fd_get(fd);
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

/* =========================================================================
 * Directory Stream System Calls
 * ========================================================================= */

int sys_opendir(const char *name) {
    if (!name) return -EFAULT;

    /* A directory is just a file opened with read permissions
     * This automatically handles path resolution, FD allocation, and
     * triggers node->ops->open() (e.g., FatFs FF_DIR allocation). */
    int fd = sys_open(name, O_RDONLY, 0);
    if (fd < 0) return fd;

    /* Verify the path resolved to an actual directory */
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !S_ISDIR(file->node->mode)) {
        sys_close(fd);
        return -ENOTDIR;
    }

    return fd;
}

int sys_readdir(int fd, struct dirent *entry) {
    if (!entry) return -EFAULT;

    /* Directory index is stored in file->offset */
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;
    if (!S_ISDIR(file->node->mode)) return -ENOTDIR;

    /* Delegate all directory reading logic to the VFS backend */
    int ret = flx_vfs_readdir(file->node, entry, file->offset);
    
    if (ret > 0) {
        file->offset++; /* Increment index on successful read */
    }
    
    return ret;
}

int sys_closedir(int fd) {
    /* Closing a directory is exactly the same as closing a file
     * This will automatically trigger node->ops->close().
     */
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

int sys_isatty(int fd) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return 0;

    return S_ISCHR(file->node->mode) ? 1 : 0;
}

off_t sys_lseek(int fd, off_t offset, int whence) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
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
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF;

    if (S_ISCHR(file->node->mode) || S_ISBLK(file->node->mode)) {
        dev_t rdev = (dev_t)((uintptr_t)file->node->data);
        FlxDev_t *dev = flx_dev_get(rdev & 0xFF);
        if (!dev) return -ENODEV;

        int ret = serial_ioctl(dev, request, arg);
        return ret; /* Let the specific error (-EINVAL, etc.) bubble up! */
    }

    return -ENOTTY;
}

/* =========================================================================
 * File Sizing System Calls
 * ========================================================================= */

int sys_truncate(const char *path, off_t length) {
    if (!path) return -EINVAL;

    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path, &remainder);
    if (!node) return -ENOENT;

    /* To truncate a file in FatFs, we must open it first! */
    if (node->ops && node->ops->open && remainder && *remainder != '\0') {
        FlxVfsNode_t *new_node = NULL;
        int res = node->ops->open(node, remainder, O_WRONLY, &new_node);
        if (res < 0) return res;
        
        res = new_node->ops->truncate(new_node, length);
        new_node->ops->close(new_node);
        return res;
    } else if (remainder && *remainder != '\0') {
        return -ENOENT;
    }

    if (node->ops && node->ops->truncate) {
        return node->ops->truncate(node, length);
    }
    return -EINVAL;
}

int sys_ftruncate(int fd, off_t length) {
    FlxOpenFile_t *file = flx_proc_fd_get(fd);
    if (!file || !file->in_use || !file->node) return -EBADF; 

    if (!(file->flags & (O_WRONLY | O_RDWR))) return -EACCES;

    if (file->node->ops && file->node->ops->truncate) {
        return file->node->ops->truncate(file->node, length);
    }
    return -EINVAL;
}

/* =========================================================================
 * Directory and Link Creation/Deletion System Calls
 * ========================================================================= */

int sys_mkdir(const char *path, mode_t mode) {
    return flx_vfs_mkdir(path, mode);
}

int sys_rmdir(const char *path) {
    return flx_vfs_rmdir(path);
}

int sys_unlink(const char *path) {
    return flx_vfs_unlink(path);
}

int sys_link(const char *oldpath, const char *newpath) {
    return flx_vfs_link(oldpath, newpath);
}

int sys_rename(const char *oldpath, const char *newpath) {
    return flx_vfs_rename(oldpath, newpath);
}

/* =========================================================================
 * File System Mount / Unmount System Calls
 * ========================================================================= */

int sys_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data)
{
    if (!source || !target || !filesystemtype) return -EINVAL;

    /* Delegate to the VFS Registry!
     * It will search for the requested filesystem type and execute its mount hook. */
    return flx_vfs_mount_type(source, target, filesystemtype, mountflags, data);
}

int sys_umount(const char *target) {
    if (!target) return -EINVAL;
    return flx_vfs_unmount(target);
}
