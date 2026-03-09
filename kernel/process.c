#include <dirent.h>
#include "process.h"
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

static FlxProcPCB_t kprocs[FLX_PROC_MAX_PROCS] = {0};

// ... existing code ...
void flx_proc_init_first(void) {
    FlxProcPCB_t *pcb_shell = &kprocs[FLX_PROC_PID_SHELL];
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

int flx_proc_fd_bind(int pid, int fd, FlxDev_t *dev) {
    if (fd_is_invalid(fd))
        return -1;
    FlxProcPCB_t *pcb = flx_proc_pcb_get(pid);
    pcb->fds[fd] = dev;
    return 0;
}


FlxDev_t *flx_proc_dev_get(int fd) {
    if (fd_is_invalid(fd))
        return NULL;
    FlxProcPCB_t *pcb = flx_proc_pcb_get(FLX_PROC_PID_SHELL);
   return pcb->fds[fd];
}

int flx_proc_getpid(void) {
    return flx_proc_pcb_get(FLX_PROC_PID_SHELL)->pid;
}

FlxVfsNode_t *flx_proc_cwd_node_get(void) {
    FlxProcPCB_t *pcb = flx_proc_pcb_get(FLX_PROC_PID_SHELL);
    return pcb->cwd;
}
FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node) {
    FlxProcPCB_t *pcb = flx_proc_pcb_get(FLX_PROC_PID_SHELL);
    return pcb->cwd = cwd_node;
}

const char *flx_proc_cwd_name_get(void) {
    FlxProcPCB_t *pcb = flx_proc_pcb_get(FLX_PROC_PID_SHELL);
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
    if (!name) return NULL;
    size_t name_len = strlen(name);
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL]; // Hardcoded to shell for now

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
    if (!name || !value || strchr(name, '=')) return -1;

    size_t name_len = strlen(name);
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL];
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
    if (!name || strchr(name, '=')) return -1;

    size_t name_len = strlen(name);
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL];

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
    /* Hardcoded to the shell PCB until multi-processing is added */
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL];
    for (int i = 0; i < FLX_PROC_MAX_DIR_STREAMS; i++) {
        if (!pcb->dir_in_use[i]) {
            pcb->dir_in_use[i] = true;
            return i;
        }
    }
    return -1; /* Out of directory file descriptors! */
}

FlxDir_t *flx_proc_fd_dir_get(int fd) {
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL];
    if (fd < 0 || fd >= FLX_PROC_MAX_DIR_STREAMS || !pcb->dir_in_use[fd]) {
        return NULL;
    }
    return &pcb->dir_streams[fd];
}

void flx_proc_fd_dir_free(int fd) {
    FlxProcPCB_t *pcb = &kprocs[FLX_PROC_PID_SHELL];
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

    /* Execute the kernel payload safely */
    if (cmd_node->file.cmd && cmd_node->file.cmd->execute) {
        return cmd_node->file.cmd->execute(argc, argv);
    }

    return -1;
}
