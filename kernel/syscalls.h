/** @file include/kernel/syscalls.h
 */
#ifndef _FLX_KERNEL_SYSCALLS_H
#define _FLX_KERNEL_SYSCALLS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/utsname.h>

struct stat;

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
mode_t sys_umask(mode_t mask);

/* Process & Time Syscalls */
int sys_getpid(void);
int sys_kill(int pid, int sig);

/* Process Priority Syscalls */
int sys_getpriority(int which, id_t who);
int sys_setpriority(int which, id_t who, int prio);

mode_t sys_umask(mode_t mask);

int sys_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int sys_chmod(const char *path, mode_t mode);
int sys_fchmod(int fd, mode_t mode);

int sys_gettimeofday(struct timeval *tv, void *tz);
int sys_settimeofday(const struct timeval *tv, const void *tz);
int sys_usleep(uint64_t usec);

/* File I/O Syscalls */
int sys_open(const char *pathname, int flags, int mode);
int sys_openat(int dirfd, const char *pathname, int flags, int mode);
int sys_close(int fd);
int sys_read(int fd, void *buf, size_t count);
int sys_write(int fd, const void *buf, size_t count);
int sys_truncate(const char *path, off_t length);
int sys_ftruncate(int fd, off_t length);

/* POSIX Directory and Link Syscalls */
int sys_lstat(const char *pathname, struct stat *statbuf);
int sys_mknod(const char *pathname, mode_t mode, dev_t dev);
int sys_mkdir(const char *path, mode_t mode);
int sys_rmdir(const char *path);
int sys_unlink(const char *path);
int sys_link(const char *oldpath, const char *newpath);
int sys_rename(const char *oldpath, const char *newpath);

int sys_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data);
int sys_umount(const char *target);

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

int sys_uname(struct utsname *buf);

#endif /* _FLX_KERNEL_SYSCALLS_H */
