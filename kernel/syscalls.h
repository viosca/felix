/* File: include/kernel/syscalls.h */
#ifndef FLX_SYSCALLS_H
#define FLX_SYSCALLS_H

#include <sys/stat.h>
#include <stddef.h>

/* =========================================================================
 * The OS Visibility Wall
 * The POSIX layer is ONLY allowed to include this file. It hides all
 * internal kernel data structures (like FlxVfsNode_t or FlxProcPCB_t).
 * ========================================================================= */

int sys_stat(const char *pathname, struct stat *statbuf);
int sys_fstat(int fd, struct stat *statbuf);
int sys_exec(const char *path, int argc, char **argv);
const char* sys_getenv(const char *name);

/* File I/O Syscalls */
int sys_open(const char *pathname, int flags, int mode);
int sys_close(int fd);
int sys_read(int fd, void *buf, size_t count);
int sys_write(int fd, const void *buf, size_t count);
int sys_ioctl(int fd, int request, void *arg);

#endif /* FLX_SYSCALLS_H */
