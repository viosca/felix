/**
 * @file process.h
 * @brief Process Control Block (PCB).
 */

#ifndef FELIX_KERNEL_PROCESS_H
#define FELIX_KERNEL_PROCESS_H

#include "kernel/devices.h"
#include "kernel/vfs.h" /* PCB needs to know about VfsNode_t */

#define FLX_PROC_MAX_PROCS 2

#define FLX_PROC_PID_KERNEL 0
#define FLX_PROC_PID_SHELL 1

#define FLX_PROC_MAX_OPEN_FILES 16
#define FLX_PROC_MAX_DIR_STREAMS 16

/* --- Environment Constraints --- */
#define FLX_PROC_MAX_ENV_VARS 16
#define FLX_PROC_MAX_ENV_LEN  128

/* Represents an open file tied to a process */
typedef struct {
    bool in_use;        /* true if fd is in use or in process of being allocated. */
    FlxVfsNode_t *node; /* NULL if this FD is closed */
    off_t offset;       /* Current read/write head */
} FlxOpenFile_t;

typedef struct {
    int pid;
    FlxOpenFile_t fds[FLX_PROC_MAX_OPEN_FILES];

    FlxDir_t dir_streams[FLX_PROC_MAX_DIR_STREAMS];

    FlxVfsNode_t *cwd; /**< Current Working Directory */

    /* Process Environment Block: Stored as "KEY=VALUE" strings */
    char env[FLX_PROC_MAX_ENV_VARS][FLX_PROC_MAX_ENV_LEN];

    /* Opaque pointer for C-Library State (Thread Local Storage).
     * The kernel blindly holds this; the stub layer manages its contents! */
    void *clib_context;

} FlxProcPCB_t;

void flx_proc_init_first(void);

/* File Descriptor Lifecycle API */
int flx_proc_fd_init(int fd);
int flx_proc_fd_alloc(void);
int flx_proc_fd_install(int fd, FlxVfsNode_t *node);
int flx_proc_fd_free(int fd);

FlxOpenFile_t *flx_proc_fd_get(int fd);
off_t flx_proc_fd_offset_get(int fd);
int flx_proc_fd_offset_set(int fd, off_t offset);

int flx_proc_getpid(void);

FlxVfsNode_t *flx_proc_cwd_node_get(void);
FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node);

const char *flx_proc_cwd_name_get(void);

char *flx_proc_getcwd(char *buf, size_t size);

/* --- Environment API --- */
char *flx_proc_getenv(const char *name);
int flx_proc_setenv(const char *name, const char *value, int overwrite);
int flx_proc_unsetenv(const char *name);

int flx_proc_fd_dir_alloc(void);
FlxDir_t *flx_proc_fd_dir_get(int fd);
void flx_proc_fd_dir_free(int fd);

/* --- System Calls --- */
int flx_proc_exec(const char *path, int argc, char **argv);

#endif /* FELIX_KERNEL_PROCESS_H */
