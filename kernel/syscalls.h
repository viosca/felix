/* File: include/kernel/syscalls.h */
#ifndef FLX_SYSCALLS_H
#define FLX_SYSCALLS_H

#include <sys/stat.h>
#include <stddef.h>
#include <dirent.h>

/* =========================================================================
 * The OS Visibility Wall
 * The POSIX layer is ONLY allowed to include this file. It hides all
 * internal kernel data structures.
 * ========================================================================= */

int sys_stat(const char *pathname, struct stat *statbuf);
int sys_fstat(int fd, struct stat *statbuf);
int sys_exec(const char *path, int argc, char **argv);
void sys_exit(int status);
const char* sys_getenv(const char *name);
int sys_setenv(const char *name, const char *value, int overwrite);
int sys_unsetenv(const char *name);

/* Process & Time Syscalls */
int sys_getpid(void);
int sys_kill(int pid, int sig);
int sys_gettimeofday(struct timeval *tv, void *tz);
int sys_settimeofday(const struct timeval *tv, const void *tz);
int sys_usleep(uint64_t usec);

/* File I/O Syscalls */
int sys_open(const char *pathname, int flags, int mode);
int sys_close(int fd);
int sys_read(int fd, void *buf, size_t count);
int sys_write(int fd, const void *buf, size_t count);
int sys_ioctl(int fd, int request, void *arg);
int sys_isatty(int fd);
off_t sys_lseek(int fd, off_t offset, int whence);

/* Directory Syscalls */
int sys_opendir(const char *name);
int sys_readdir(int fd, struct dirent *entry);
int sys_closedir(int fd);

long sys_telldir(int fd);
int sys_seekdir(int fd, long loc);
int sys_rewinddir(int fd);

/* Directory & Path Syscalls */

int sys_chdir(const char *path);
char *sys_getcwd(char *buf, size_t size);

/* Memory Management Syscalls */
void *sys_sbrk(ptrdiff_t incr);

#endif /* FLX_SYSCALLS_H */
