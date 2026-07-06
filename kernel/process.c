#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "kernel/process.h"
#include "kernel/sched.h"
#include "kernel/binfmt.h"
#include "kernel/kmalloc.h"
#include "kernel/klog.h"

static inline FlxProcPCB_t *get_current_pcb(void) {
    return (FlxProcPCB_t *)sched_get_tls();
    //return g_current_task;
}

/* =========================================================================
 * Internal Static Helpers (DRY bounds checking)
 * ========================================================================= */

static inline FlxVfsFile_t *get_file_desc(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || fd < 0 || fd >= FLX_PROC_MAX_OPEN_FILES) return NULL;
    return &pcb->fds[fd];
}

static inline int find_env_slot(FlxProcPCB_t *pcb, const char *name, size_t name_len) {
    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        if (pcb->env[i][0] != '\0' &&
            strncmp(pcb->env[i], name, name_len) == 0 &&
            pcb->env[i][name_len] == '=') {
            return i;
        }
    }
    return -1;
}

/* =========================================================================
 * Process Initialization & Teardown
 * ========================================================================= */

void flx_proc_init_current(int pid) {
    /* Dynamically allocate the PCB on the kernel heap */
    FlxProcPCB_t *pcb = kzalloc(sizeof(FlxProcPCB_t));
    if (!pcb) {
        kprintf("\r\n[KERNEL PANIC] Out of memory allocating PCB for PID %d!\r\n", pid);
        while(1);
    }

    pcb->pid = pid;

    /* Inject the new PCB into the currently executing FreeRTOS TCB */
    sched_set_tls((void *)pcb);

    /* (kzalloc already zeroed the fds and env arrays, so we just set defaults) */
    pcb->pwd = flx_vfs_root_get();

    flx_proc_setenv("TERM", "vt100", 1);
    flx_proc_setenv("PATH", "/bin", 1);
    flx_proc_setenv("PWD", "/", 1);
}

void flx_proc_exit(int status) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (pcb) {
        /* Close all open file descriptors */
        for (int i = 0; i < FLX_PROC_MAX_OPEN_FILES; i++) {
            if (pcb->fds[i].in_use && pcb->fds[i].node) {
                FlxVfsNode_t *node = pcb->fds[i].node;

                /* Notify the specific driver (FATFS, UART, etc.) that the file is closing */
                if (node->ops && node->ops->close) {
                    node->ops->close(node);
                }

                pcb->fds[i].in_use = false;
                pcb->fds[i].node = NULL;
            }
        }

        /* Free C-Library context if the newlib stub allocated one */
        if (pcb->clib_context) {
            kfree(pcb->clib_context);
        }

        /* Safety First: Clear the TLS pointer BEFORE freeing the memory
         * to prevent ISRs or context switches from grabbing a dangling pointer. */
        sched_set_tls(NULL);

        /* Free the PCB memory back to the kernel heap */
        kfree(pcb);
    }

    /* Safely destroy the FreeRTOS task */
    sched_thread_exit();
}
/* =========================================================================
 * File Descriptor Management
 * ========================================================================= */
int flx_proc_fd_init(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    pcb->fds[fd].in_use = true; /* RESERVE IT instantly! */
    pcb->fds[fd].node = NULL;
    pcb->fds[fd].offset = 0;

    return fd;
}

int flx_proc_fd_alloc(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    for (int i = 3; i < FLX_PROC_MAX_OPEN_FILES; i++) {
        if (pcb->fds[i].in_use) continue;
        flx_proc_fd_init(i);
        return i;
    }
    return -1;
}

int flx_proc_fd_install(int fd, FlxVfsNode_t *node) {
    FlxVfsFile_t *file = get_file_desc(fd);
    if (!file || !node) return -1;

    file->node = node;
    file->offset = 0;
    return 0;
}

int flx_proc_fd_free(int fd) {
    FlxVfsFile_t *file = get_file_desc(fd);
    if (!file) return -1;

    file->in_use = false;
    file->node = NULL;
    file->offset = 0;
    return 0;
}

FlxVfsFile_t *flx_proc_fd_get(int fd) {
    return get_file_desc(fd);
}

 off_t flx_proc_fd_offset_get(int fd) {
     FlxVfsFile_t *file = get_file_desc(fd);
     return (file && file->in_use) ? file->offset : (off_t)-1;
 }

 int flx_proc_fd_offset_set(int fd, off_t offset) {
     FlxVfsFile_t *file = get_file_desc(fd);
     if (!file || !file->in_use || offset < 0) return -1;

     file->offset = offset;
     return 0;
 }

 mode_t flx_proc_umask_get(void) {
     FlxProcPCB_t *pcb = get_current_pcb();
     if (!pcb) return 0022; // Safe fallback
     return pcb->umask;
 }

 mode_t flx_proc_umask_set(mode_t new_umask) {
     FlxProcPCB_t *pcb = get_current_pcb();
     if (!pcb) return 0022;
     mode_t saved_umask = pcb->umask;
     pcb->umask = new_umask & 0777; /* Restrict to valid permission bits */
     return saved_umask;
 }


/* =========================================================================
 * Process Information
 * ========================================================================= */

int flx_proc_getpid(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    return pcb ? pcb->pid : -1;
}

FlxVfsNode_t *flx_proc_cwd_node_get(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    return pcb ? pcb->pwd : NULL;
}

FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;
    return pcb->pwd = cwd_node;
}

const char *flx_proc_cwd_name_get(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    if (pcb->pwd) return pcb->pwd->name;
    return "/";
}

char *flx_proc_getcwd(char *buf, size_t size) {
    if (!buf || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    FlxVfsNode_t *cwd_node = flx_proc_cwd_node_get();
    if (!cwd_node) {
        errno = ENOENT;
        return NULL;
    }

    /* Ask the VFS layer to traverse the tree and build the full path */
    char *ret = flx_vfs_get_path(cwd_node, buf, size);
    if (!ret) {
        /* flx_vfs_get_path returns NULL if the buffer size is too small */
        errno = ERANGE;
        return NULL;
    }

    return buf;
}
/* =========================================================================
 * Process Environment Management
 * ========================================================================= */

char *flx_proc_getenv(const char *name) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || !name) return NULL;

    size_t name_len = strlen(name);
    int idx = find_env_slot(pcb, name, name_len);

    if (idx >= 0) {
        return &pcb->env[idx][name_len + 1];
    }
    return NULL;
}

int flx_proc_setenv(const char *name, const char *value, int overwrite) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || !name || !value || strchr(name, '=')) return -EINVAL;

    size_t name_len = strlen(name);
    int idx = find_env_slot(pcb, name, name_len);

    /* Update existing if found and overwrite is permitted */
    if (idx >= 0) {
        if (!overwrite) return 0;
        snprintf(pcb->env[idx], FLX_PROC_MAX_ENV_LEN, "%s=%s", name, value);
        return 0;
    }

    /* Otherwise, find an empty slot */
    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        if (pcb->env[i][0] == '\0') {
            snprintf(pcb->env[i], FLX_PROC_MAX_ENV_LEN, "%s=%s", name, value);
            return 0;
        }
    }
    return -ENOMEM; /* Out of environment space */
}

int flx_proc_unsetenv(const char *name) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || !name || strchr(name, '=')) return -EINVAL;

    int idx = find_env_slot(pcb, name, strlen(name));
    if (idx >= 0) {
        pcb->env[idx][0] = '\0';
    }
    return 0;
}

/* =========================================================================
 * System Calls
 * ========================================================================= */

int flx_proc_exec(const char *path, int argc, char **argv) {
    const char *remainder = NULL;
    FlxVfsNode_t *cmd_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path, &remainder);

    /* 1. Guard: Reject invalid paths AND dynamic filesystem boundaries.
     * If remainder has text, they are trying to execute a file off the SD card,
     * which we cannot do until an ELF loader is implemented! */
    if (!cmd_node || (remainder && *remainder != '\0')) {
        return -1;
    }

    /* 2. Guard: Must be a regular file, not a directory */
    if (!S_ISREG(cmd_node->mode)) {
        return -1;
    }

    /* 3. Safe to execute: cmd_node->data is guaranteed to be an in-memory payload */
    return flx_binfmt_load(cmd_node->data, argc, argv);
}
