/* @file felix/fs/fatfs/fatfs_vfs.c
 * Namespace isolation: FatFs defines 'DIR', which conflicts with POSIX dirent.h.
 * We rename the FatFs version to 'FF_DIR' locally.
 */
#define DIR FF_DIR
#include "ff.h"
#undef DIR

#include "kernel/vfs.h"
#include "kernel/kmalloc.h"
#include "fs/fatfs/fatfs_vfs.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/* Track the underlying VFS block node for each FatFs logical drive (0-9) */
#define FATFS_MAX_VOLUMES 10
static FlxVfsNode_t *g_fatfs_blk_nodes[FATFS_MAX_VOLUMES] = {0};

/* Exposed for fatfs_diskio.c */
FlxVfsNode_t* fatfs_get_blk_node(BYTE pdrv) {
    if (pdrv >= FATFS_MAX_VOLUMES) return NULL;
    return g_fatfs_blk_nodes[pdrv];
}

#if FF_MULTI_PARTITION
#error "Please disable FF_MULTI_PARTITION in ffconf.h. Felix OS block manager handles partitions natively!"
#endif

/* Use FatFs's internal Long File Name constant (typically 255) + 1 for null terminator */
#ifndef FF_MAX_LFN
#define FF_MAX_LFN 255 /* Fallback in case LFN is disabled in ffconf.h */
#endif
#define FATFS_MAX_PATH (FF_MAX_LFN + 1)

/**
 * @brief Helper: Map FatFs errors to POSIX errno
 */
static int fat_to_errno(FRESULT res) {
    switch (res) {
        case FR_OK:                  return 0;
        case FR_NO_FILE:
        case FR_NO_PATH:             return -ENOENT;
        case FR_DENIED:              return -EACCES;
        case FR_EXIST:               return -EEXIST;
        case FR_INVALID_OBJECT:      return -EBADF;
        case FR_WRITE_PROTECTED:     return -EROFS;
        case FR_NOT_READY:           return -ENODEV;
        case FR_DISK_ERR:            return -EIO;
        case FR_INT_ERR:             return -EFAULT;
        case FR_NOT_ENABLED:         return -ENXIO;
        case FR_NO_FILESYSTEM:       return -ENODEV;
        case FR_TIMEOUT:             return -ETIMEDOUT;
        case FR_LOCKED:              return -EBUSY;
        case FR_NOT_ENOUGH_CORE:     return -ENOMEM;
        case FR_TOO_MANY_OPEN_FILES: return -ENFILE;
        default:                     return -EIO;
    }
}

/* =========================================================================
 * VFS Operations for FatFs Files & Directories
 * ========================================================================= */

/* Forward declaration of the operations table to resolve circular dependencies */
static const FlxVfsOps_t fatfs_vfs_ops;

static int fatfs_vfs_open(FlxVfsNode_t *node, const char *path, int flags, FlxVfsNode_t **out_node) {
    if (!node || !out_node) return -EINVAL;

    bool is_root = (!path || path[0] == '\0');
    
    /* Move huge path buffer to the heap */
    char *fat_path = kmalloc(FATFS_MAX_PATH);
    if (!fat_path) return -ENOMEM;
    
    /* Build the FatFs path. If it's the root, ensure a trailing slash for safety. */
    if (is_root) {
        snprintf(fat_path, FATFS_MAX_PATH, "%s/", node->name);
    } else {
        snprintf(fat_path, FATFS_MAX_PATH, "%s/%s", node->name, path);
    }

    BYTE fatfs_flags = 0;
    int acc = (flags & O_ACCMODE);
    if (acc == O_RDONLY) fatfs_flags |= FA_READ;
    if (acc == O_WRONLY) fatfs_flags |= FA_WRITE;
    if (acc == O_RDWR)   fatfs_flags |= (FA_READ | FA_WRITE);
    
    if (flags & O_CREAT)  fatfs_flags |= FA_OPEN_ALWAYS;
    if (flags & O_TRUNC)  fatfs_flags |= FA_CREATE_ALWAYS;
    if (flags & O_APPEND) fatfs_flags |= FA_OPEN_APPEND;

    /* Dynamically allocate a brand new VFS Node for this specific file! */
    FlxVfsNode_t *file_node = kzalloc(sizeof(FlxVfsNode_t));
    if (!file_node) {
        kfree(fat_path);
        return -ENOMEM;
    }
    
    file_node->name = kstrdup(path ? path : "");
    file_node->kflags = FLX_VFS_KFLAG_ALLOCATED_NAME | FLX_VFS_KFLAG_ALLOCATED_NODE;
    file_node->parent = node; /* Link it back to the /mnt directory */
    file_node->ops = &fatfs_vfs_ops;

    /* Move huge FILINFO struct (500+ bytes) to the heap */
    FILINFO *fno = kmalloc(sizeof(FILINFO));
    if (!fno) {
        kfree((void*)file_node->name);
        kfree(file_node);
        kfree(fat_path);
        return -ENOMEM;
    }

    FRESULT res = FR_OK;
    bool is_dir = is_root; /* If it's the root path, it is guaranteed to be a directory */

    /* FatFs f_stat cannot query the root directory, so we skip it if is_root == true */
    if (!is_root) {
        res = f_stat(fat_path, fno);
        if (res == FR_OK) {
            is_dir = (fno->fattrib & AM_DIR) != 0;
        }
    }

    int ret_errno = 0;

    /* Branch based on whether we are opening a directory or a file */
    if (is_dir && (is_root || res == FR_OK)) {
        FF_DIR *dj = kmalloc(sizeof(FF_DIR));
        res = f_opendir(dj, fat_path);
        if (res != FR_OK) { 
            kfree(dj); 
            ret_errno = fat_to_errno(res);
            goto cleanup_error;
        }
        
        file_node->mode = S_IFDIR | 0755;
        file_node->data = dj;
    } else if (res == FR_OK || (res == FR_NO_FILE && (flags & O_CREAT))) {
        FIL *fp = kmalloc(sizeof(FIL));
        res = f_open(fp, fat_path, fatfs_flags);
        if (res != FR_OK) { 
            kfree(fp); 
            ret_errno = fat_to_errno(res);
            goto cleanup_error;
        }
        
        file_node->mode = S_IFREG | 0666;
        file_node->data = fp;
    } else {
        ret_errno = fat_to_errno(res);
        goto cleanup_error;
    }

    kfree(fno);
    kfree(fat_path);
    *out_node = file_node;
    return 0;

cleanup_error:
    kfree(fno);
    kfree((void*)file_node->name);
    kfree(file_node);
    kfree(fat_path);
    return ret_errno;
}

static int fatfs_vfs_close(FlxVfsNode_t *node) {
    if (!node || !node->data) return -EBADF;
    
    /* Safety check: If this node has no parent, it's the mount root.
     * Its 'data' is the FATFS structure, which must NOT be closed or freed here! */
    if (node->parent == NULL || node->parent->ops != node->ops) {
        return 0; 
    }

    FRESULT res;
    if (S_ISDIR(node->mode)) {
        res = f_closedir((FF_DIR *)node->data);
    } else {
        res = f_close((FIL *)node->data);
    }

    kfree(node->data);
    
    /* FREE THE DYNAMIC NODE ITSELF! */
    if (node->kflags & FLX_VFS_KFLAG_ALLOCATED_NAME) kfree((void*)node->name);
    kfree(node);

    return fat_to_errno(res);
}

static ssize_t fatfs_vfs_read(FlxFile_t *file, void *buf, size_t count) {
    if (!file || !file->node || !file->node->data) return -EBADF;
    if (S_ISDIR(file->node->mode)) return -EISDIR;

    FIL *fp = (FIL *)file->node->data;
    UINT br;

    /* Move the FatFs internal pointer to match the VFS offset */
    FRESULT res = f_lseek(fp, (FSIZE_t)file->offset);
    if (res != FR_OK) return fat_to_errno(res);

    res = f_read(fp, buf, (UINT)count, &br);
    if (res != FR_OK) return fat_to_errno(res);

    /* Automatically advance the VFS offset */
    file->offset += br;
    return (ssize_t)br;
}

static ssize_t fatfs_vfs_write(FlxFile_t *file, const void *buf, size_t count) {
    if (!file || !file->node || !file->node->data) return -EBADF;
    if (S_ISDIR(file->node->mode)) return -EISDIR;

    FIL *fp = (FIL *)file->node->data;
    UINT bw;

    /* Move the FatFs internal pointer to match the VFS offset */
    FRESULT res = f_lseek(fp, (FSIZE_t)file->offset);
    if (res != FR_OK) return fat_to_errno(res);

    res = f_write(fp, buf, (UINT)count, &bw);
    if (res != FR_OK) return fat_to_errno(res);

    /* Automatically advance the VFS offset */
    file->offset += bw;
    return (ssize_t)bw;
}


static int fatfs_vfs_readdir(FlxVfsNode_t *node, struct dirent *entry, int index) {
    if (!node || !node->data) return -EBADF;
    if (!S_ISDIR(node->mode)) return -ENOTDIR;
    
    FF_DIR *dj = (FF_DIR *)node->data;
    
    FILINFO *fno = kmalloc(sizeof(FILINFO));
    if (!fno) return -ENOMEM;

    /* FatFs readdir is sequential. */
    FRESULT res = f_readdir(dj, fno);
    if (res != FR_OK || fno->fname[0] == 0) {
        kfree(fno);
        return -1; /* End of directory */
    }

    /* FIX: POSIX ls ignores files where d_ino == 0 (marks deleted files). 
     * We must generate a stable, non-zero pseudo-inode from the filename. */
    unsigned long hash = 5381;
    for (int i = 0; fno->fname[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + fno->fname[i];
    }
    entry->d_ino = (hash == 0) ? 1 : hash; /* Must NEVER be 0 */

    strncpy(entry->d_name, fno->fname, sizeof(entry->d_name) - 1);
    entry->d_name[sizeof(entry->d_name) - 1] = '\0';
    entry->d_type = (fno->fattrib & AM_DIR) ? DT_DIR : DT_REG;

    kfree(fno);
    return 0;
}

static int fatfs_vfs_getattr(FlxVfsNode_t *node, const char *path, struct stat *st) {
    if (!node || !st) return -EINVAL;
    memset(st, 0, sizeof(struct stat));

    /* If path is empty, we are stating the mount root itself! */
    if (!path || path[0] == '\0') {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    char *fat_path = kmalloc(FATFS_MAX_PATH);
    if (!fat_path) return -ENOMEM;
    snprintf(fat_path, FATFS_MAX_PATH, "%s/%s", node->name, path);

    FILINFO *fno = kmalloc(sizeof(FILINFO));
    if (!fno) {
        kfree(fat_path);
        return -ENOMEM;
    }

    FRESULT res = f_stat(fat_path, fno);
    if (res != FR_OK) {
        kfree(fno);
        kfree(fat_path);
        return fat_to_errno(res);
    }

    st->st_mode = (fno->fattrib & AM_DIR) ? (S_IFDIR | 0755) : (S_IFREG | 0666);
    st->st_size = fno->fsize;
    st->st_nlink = 1;
    st->st_uid = 0; 
    st->st_gid = 0; 

    kfree(fno);
    kfree(fat_path);
    return 0;
}

static int fatfs_vfs_unmount(FlxVfsNode_t *node) {
    if (!node || !node->data) return -EINVAL;

    /* Here we actually want to detach the drive and free the work area */
    FATFS *fs = (FATFS *)node->data;
    FRESULT res = f_mount(NULL, node->name, 0); 
    
    /* Clear the block device mapping */
    if (node->name && node->name[0] >= '0' && node->name[0] <= '9') {
        int drive_num = node->name[0] - '0';
        g_fatfs_blk_nodes[drive_num] = NULL;
    }

    kfree(fs);
    node->data = NULL;

    return fat_to_errno(res);
}

static int fatfs_vfs_truncate(FlxVfsNode_t *node, off_t length) {
    if (!node || !node->data) return -EBADF;
    if (S_ISDIR(node->mode)) return -EISDIR;
	
    FIL *fp = (FIL *)node->data;

    /* Move pointer to the requested length, then chop it off */
    FRESULT res = f_lseek(fp, (FSIZE_t)length);
	if (res != FR_OK) {
		return fat_to_errno(res);
	}
    return fat_to_errno(f_truncate(fp));
}

/* The operations table */
static const FlxVfsOps_t fatfs_vfs_ops = {
    .getattr = fatfs_vfs_getattr,
    .open    = fatfs_vfs_open,
    .read    = fatfs_vfs_read,
    .write   = fatfs_vfs_write,
    .close   = fatfs_vfs_close,
    .unmount = fatfs_vfs_unmount,
    .truncate = fatfs_vfs_truncate,
    .readdir = fatfs_vfs_readdir
};

/* =========================================================================
 * Filesystem Registry Hook
 * ========================================================================= */

/**
 * @brief Initializes FatFs and returns the Root Node to the VFS layer via out_node.
 * @return 0 on success, or a negative POSIX error code.
 */
static int fatfs_mount_hook(const char *source, unsigned long mountflags, const void *data, FlxVfsNode_t **out_node) {
    if (!source || !out_node) return -EINVAL;

    /* Resolve the source path to get the actual block device node */
    const char *remainder = NULL;
    FlxVfsNode_t *blk_node = flx_vfs_path_resolve(flx_vfs_root_get(), source, &remainder);

    if (!blk_node || (remainder && *remainder != '\0')) {
        return -ENODEV;
    }

    /* Translate UNIX block device path to FatFs logical drive string */
    char logical_drive[4] = {0};
    int drive_num = -1;

    if (strcmp(source, "/dev/sd0") == 0) {
        /* Standard auto-detect or full unpartitioned disk */
        drive_num = 0;
    } else if (strncmp(source, "/dev/sd0p", 9) == 0) {
        /* Parse partitions: /dev/sd0p1, /dev/sd0p2, etc. */
        int part_num = source[9] - '0';
        if (part_num >= 1 && part_num <= 9) {
            /* Map Partition 1 to logical drive "0:", Partition 2 to "1:", etc. */
            drive_num = part_num - 1;
        }
    } else if (source[0] >= '0' && source[0] <= '9' && source[1] == ':' && source[2] == '\0') {
        /* Allow direct FatFs logical drive strings (e.g. "0:", "1:") to support
         * legacy mounting or future table-driven mapping (like /etc/fstab) */
        drive_num = source[0] - '0';
    }

    if (drive_num >= 0 && drive_num <= 9) {
        /* Format the FatFs logical drive string (e.g., "0:", "1:") */
        logical_drive[0] = '0' + drive_num;
        logical_drive[1] = ':';
        logical_drive[2] = '\0';

        /* Save the block node so diskio.c can read from it! */
        g_fatfs_blk_nodes[drive_num] = blk_node;
    } else {
        /* If we don't recognize the block device/partition, reject the mount */
        return -ENODEV;
    }

    /* Allocate work area */
    FATFS *fs = kmalloc(sizeof(FATFS));
    if (!fs) return -ENOMEM;

    /* Mount immediately using the translated logical drive */
    FRESULT res = f_mount(fs, logical_drive, 1);
    if (res != FR_OK) {
        kfree(fs);
        return fat_to_errno(res);
    }

    /* Create the Root Node representing the mounted filesystem */
    FlxVfsNode_t *sd_node = kzalloc(sizeof(FlxVfsNode_t));
    if (!sd_node) {
        f_mount(NULL, logical_drive, 0);
        kfree(fs);
        return -ENOMEM;
    }

    /* Copy the FatFs logical drive into the node name so f_open() can format paths correctly! */
    sd_node->name = kstrdup(logical_drive);
    if (sd_node->name) {
        sd_node->kflags |= (FLX_VFS_KFLAG_ALLOCATED_NAME |
                            FLX_VFS_KFLAG_ALLOCATED_NODE |
                            FLX_VFS_KFLAG_OPAQUE_NODE);
    } else {
        f_mount(NULL, logical_drive, 0);
        kfree(fs);
        kfree(sd_node);
        return -ENOMEM;
    }

    sd_node->mode = S_IFDIR | 0755;
    sd_node->ops = &fatfs_vfs_ops;
    sd_node->data = fs;

    /* Hand it back to the VFS Core! */
    *out_node = sd_node;
    return 0;
}

/* FIX 3: Wire the new unmount hook into your VFS registry struct */
static FlxFsType_t fatfs_type = {
    .name = "fatfs",
    .mount = fatfs_mount_hook,
    .next = NULL
};

/**
 * @brief Register FatFs with the kernel. Call this exactly once during boot.
 */
void fatfs_vfs_register(void) {
    flx_vfs_register_fs(&fatfs_type);
}
