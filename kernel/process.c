#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "process.h"
#include "kernel/binfmt.h"

static FlxProcPCB_t kprocs[FLX_PROC_MAX_PROCS] = {0};

/* The global pointer to the currently running task */
FlxProcPCB_t *g_current_task = NULL;

/**
 * @brief Internal accessor for the current task.
 * Future-proofs the OS for SMP (Multi-core) by centralizing how the
 * active PCB is retrieved.
 */
static inline FlxProcPCB_t *get_current_pcb(void) {
    return g_current_task;
}


// ... existing code ...
void flx_proc_init_first(void) {
    FlxProcPCB_t *pcb_shell = &kprocs[FLX_PROC_PID_SHELL];

    /* Set the current task to the shell so all POSIX calls route here! */
    g_current_task = pcb_shell;

    pcb_shell->pid = FLX_PROC_PID_SHELL;

    for (int i = 0; i < FLX_PROC_MAX_OPEN_FILES; i++) {
        pcb_shell->fds[i] = NULL;
    }

    /* Initialize directory streams */
    for (int i = 0; i < FLX_PROC_MAX_DIR_STREAMS; i++) {
        pcb_shell->dir_in_use[i] = false;
    }

    /* Initialize empty environment */
    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        pcb_shell->env[i][0] = '\0';
    }

    /* Initialize VFS and set CWD to Root */
    pcb_shell->cwd = flx_vfs_root_init();

    /* Pre-populate standard environment variables! */
    flx_proc_setenv("TERM", "vt100", 1); /* Keeps linenoise in smart mode */
    flx_proc_setenv("PATH", "/bin", 1);
    flx_proc_setenv("PWD", "/", 1);      /* Kernel bootstraps the working directory */
}

static inline bool fd_is_valid(int fd) {
    return (fd >= 0 && fd <= FLX_PROC_MAX_OPEN_FILES);
}

static inline bool fd_is_invalid(int fd) {
    return !fd_is_valid(fd);
}

FlxProcPCB_t *flx_proc_pcb_get(int pid) {
    return &kprocs[pid];
}

int flx_proc_fd_bind(int fd, FlxDev_t *dev) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    if (fd_is_invalid(fd))
        return -1;

    pcb->fds[fd] = dev;
    return 0;
}


FlxDev_t *flx_proc_fd_dev_get(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    if (fd < 0 || fd >= FLX_PROC_MAX_OPEN_FILES) return NULL;

    return pcb->fds[fd];
}

int flx_proc_getpid(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    return pcb->pid;
}

FlxVfsNode_t *flx_proc_cwd_node_get(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    return pcb->cwd;
}
FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    return pcb->cwd = cwd_node;
}

const char *flx_proc_cwd_name_get(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    if (pcb->cwd) {
        return kprocs[FLX_PROC_PID_SHELL].cwd->name;
    }
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

    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        /* Check if slot is active and starts with "KEY=" */
        if (pcb->env[i][0] != '\0' &&
            strncmp(pcb->env[i], name, name_len) == 0 &&
            pcb->env[i][name_len] == '=') {

            /* Return pointer to the start of the VALUE */
            return &pcb->env[i][name_len + 1];
        }
    }
    return NULL;
}

int flx_proc_setenv(const char *name, const char *value, int overwrite) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || !name || !value || strchr(name, '=')) return -1;

    size_t name_len = strlen(name);

    int empty_slot = -1;

    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        if (pcb->env[i][0] == '\0') {
            if (empty_slot == -1) empty_slot = i;
            continue;
        }

        /* If variable already exists */
        if (strncmp(pcb->env[i], name, name_len) == 0 && pcb->env[i][name_len] == '=') {
            if (!overwrite) return 0;
            snprintf(pcb->env[i], FLX_PROC_MAX_ENV_LEN, "%s=%s", name, value);
            return 0;
        }
    }

    /* Variable doesn't exist, use the first empty slot */
    if (empty_slot != -1) {
        snprintf(pcb->env[empty_slot], FLX_PROC_MAX_ENV_LEN, "%s=%s", name, value);
        return 0;
    }

    /* Out of environment slots! */
    return -1;
}

int flx_proc_unsetenv(const char *name) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb || !name || strchr(name, '=')) return -1;

    size_t name_len = strlen(name);

    for (int i = 0; i < FLX_PROC_MAX_ENV_VARS; i++) {
        if (pcb->env[i][0] != '\0' &&
            strncmp(pcb->env[i], name, name_len) == 0 &&
            pcb->env[i][name_len] == '=') {

            /* Null out the first character to mark slot as free */
            pcb->env[i][0] = '\0';
            return 0;
        }
    }
    return 0; /* POSIX says return 0 if it wasn't found anyway */
}

/* --- Directory Stream Management --- */

int flx_proc_fd_dir_alloc(void) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return -1;

    for (int i = 0; i < FLX_PROC_MAX_DIR_STREAMS; i++) {
        if (!pcb->dir_in_use[i]) {
            pcb->dir_in_use[i] = true;
            return i;
        }
    }
    return -1; /* Out of directory file descriptors! */
}

FlxDir_t *flx_proc_fd_dir_get(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return NULL;

    if (fd < 0 || fd >= FLX_PROC_MAX_DIR_STREAMS || !pcb->dir_in_use[fd]) {
        return NULL;
    }
    return &pcb->dir_streams[fd];
}

void flx_proc_fd_dir_free(int fd) {
    FlxProcPCB_t *pcb = get_current_pcb();
    if (!pcb) return;

    if (fd >= 0 && fd < FLX_PROC_MAX_DIR_STREAMS) {
        pcb->dir_in_use[fd] = false;
    }
}

/* --- System Calls --- */

int flx_proc_exec(const char *path, int argc, char **argv) {
    /* For now, this is a thin wrapper that safely crosses into the VFS.
     * In the future, this is where memory cloning (fork) or tasking (yield) will live! */

    FlxVfsNode_t *cmd_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), path);

    /* Ensure the node exists and is actually a regular file/executable */
    if (!cmd_node || !S_ISREG(cmd_node->mode)) {
        return -1;
    }

    /* Hand the file's raw data payload over to the Binary Format Loader! */
    return flx_load_binary(cmd_node->data, argc, argv);
}
