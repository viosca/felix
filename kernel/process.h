/**
 * @file process.h
 * @brief Process Control Block (PCB).
 */

#ifndef FELIX_KERNEL_PROCESS_H
#define FELIX_KERNEL_PROCESS_H
#include <sys/types.h>
#include "kernel/devices.h"
#include "kernel/vfs.h" /* PCB needs to know about VfsNode_t and FlxVfsFile_t */

//#define FLX_PROC_MAX_PROCS 2

#define FLX_PROC_PID_KERNEL 0
#define FLX_PROC_PID_INIT 1

#define FLX_PROC_MAX_OPEN_FILES 32

/* --- Environment Constraints --- */
#define FLX_PROC_MAX_ENV_VARS 16
#define FLX_PROC_MAX_ENV_LEN  128

typedef struct {
    int pid; /**< Process Id */
    FlxVfsFile_t fds[FLX_PROC_MAX_OPEN_FILES]; /**< Per process list of open files */
    FlxVfsNode_t *pwd; /**< Current Working Directory */
    char env[FLX_PROC_MAX_ENV_VARS][FLX_PROC_MAX_ENV_LEN]; /**< Process Environment Block: Stored as "KEY=VALUE" strings */
    mode_t umask; /**< The process-specific file creation mask */
    void *clib_context; /**< Opaque pointer for C-Library State (Thread Local Storage) */
} FlxProcPCB_t;

void flx_proc_init_first(void);

/* File Descriptor Lifecycle API */
int flx_proc_fd_init(int fd);
int flx_proc_fd_alloc(void);
int flx_proc_fd_install(int fd, FlxVfsNode_t *node);
int flx_proc_fd_free(int fd);

FlxVfsFile_t *flx_proc_fd_get(int fd);
off_t flx_proc_fd_offset_get(int fd);
int flx_proc_fd_offset_set(int fd, off_t offset);

mode_t flx_proc_umask_get(void);
mode_t flx_proc_umask_set(mode_t umask);

int flx_proc_getpid(void);

FlxVfsNode_t *flx_proc_cwd_node_get(void);
FlxVfsNode_t *flx_proc_cwd_node_set(FlxVfsNode_t *cwd_node);

const char *flx_proc_cwd_name_get(void);

char *flx_proc_getcwd(char *buf, size_t size);

/* --- Environment API --- */
char *flx_proc_getenv(const char *name);
int flx_proc_setenv(const char *name, const char *value, int overwrite);
int flx_proc_unsetenv(const char *name);

/* --- System Calls --- */
int flx_proc_exec(const char *path, int argc, char **argv);

void flx_proc_init_current(int pid);
void flx_proc_exit(int status);

#endif /* FELIX_KERNEL_PROCESS_H */
