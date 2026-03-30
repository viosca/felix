#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "kernel/process.h"
#include "kernel/binfmt.h"

static FlxProcPCB_t kprocs[FLX_PROC_MAX_PROCS] = {0};

/* The global pointer to the currently running task */
FlxProcPCB_t *g_current_task = NULL;

static inline FlxProcPCB_t *get_current_pcb(void) {
    return g_current_task;
}

/* =========================================================================
 * Internal Static Helpers (DRY bounds checking)
 * ========================================================================= */

static inline FlxOpenFile_t *get_file_desc(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || fd < 0 || fd >= FLX_PROC_MAX_OPEN_FILES) return NULL;
    return &pcb->fds[fd];
}

static inline FlxDir_t *get_dir_desc(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || fd < 0 || fd >= FLX_PROC_MAX_DIR_STREAMS) return NULL;
    return &pcb->dir_streams[fd];
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
 * Process Initialization
 * ========================================================================= */

void flx_proc_init_first(void) {
    FlxProcPCB_t *pcb_shell = &kprocs[FLX_PROC_PID_SHELL];
    g_current_task = pcb_shell;
    pcb_shell->pid = FLX_PROC_PID_SHELL;

    for (int i = 0; i < FLX_PROC_MAX_OPEN_FILES; i++) {
        pcb_shell->fds[i].in_use = false;
        pcb_shell->fds[i].node = NULL;
        pcb_shell->fds[i].offset = 0;
    }

    for (int i = 0; i < FLX_PROC_MAX_DIR_STREAMS; i++) {
        pcb_shell->dir_streams[i].in_use = false;
    }

    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        pcb_shell->env[i][0] = '\0';
    }

    pcb_shell->cwd = flx_vfs_root_init();

    flx_proc_setenv("TERM", "vt100", 1);
    flx_proc_setenv("PATH", "/bin", 1);
    flx_proc_setenv("PWD", "/", 1);
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
    FlxOpenFile_t *file = get_file_desc(fd);
    if (!file || !node) return -1;

    file->node = node;
    file->offset = 0;
    return 0;
}

int flx_proc_fd_free(int fd) {
    FlxOpenFile_t *file = get_file_desc(fd);
    if (!file) return -1;

    file->in_use = false;
    file->node = NULL;
    file->offset = 0;
    return 0;
}

FlxOpenFile_t *flx_proc_fd_get(int fd) {
    return get_file_desc(fd);
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
    return pcb ? pcb->cwd : NULL;
}

FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;
    return pcb->cwd = cwd_node;
}

const char *flx_proc_cwd_name_get(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    if (pcb->cwd) return pcb->cwd->name;
    return "/";
}

char *flx_proc_getcwd(char *buf, size_t size) {
    if (!buf || !size) {
        errno = ENOENT;
        return NULL;
    }
    const char *node_name = flx_proc_cwd_name_get();
    if (strlen(node_name) >= size) {
        errno = ERANGE;
        return NULL;
    }
    strncpy(buf, node_name, size);
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
 * Directory Stream Management
 * ========================================================================= */

int flx_proc_fd_dir_alloc(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    for (int i = 0; i < FLX_PROC_MAX_DIR_STREAMS; i++) {
        if (!pcb->dir_streams[i].in_use) {
            pcb->dir_streams[i].in_use = true;
            return i;
        }
    }
    return -1;
}

FlxDir_t *flx_proc_fd_dir_get(int fd) {
    FlxDir_t *dir = get_dir_desc(fd);
    if (dir && dir->in_use) return dir;
    return NULL;
}

void flx_proc_fd_dir_free(int fd) {
    FlxDir_t *dir = get_dir_desc(fd);
    if (dir) dir->in_use = false;
}

/* =========================================================================
 * System Calls
 * ========================================================================= */

int flx_proc_exec(const char *path, int argc, char **argv) {
    FlxVfsNode_t *cmd_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path);

    if (!cmd_node || !S_ISREG(cmd_node->mode)) {
        return -1;
    }
    return flx_binfmt_load(cmd_node->data, argc, argv);
}
