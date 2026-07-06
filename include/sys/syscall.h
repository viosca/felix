/**
 * @file felix/include/sys/syscall.h
 * @brief POSIX raw syscall extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_SYSCALL_H
#define _FLX_INCLUDE_SYS_SYSCALL_H

/* Define common Linux syscall numbers expected by Toybox optimizations.
 * Mapped to the standard RISC-V 64-bit (asm-generic) ABI. */

/* Directory & Path Operations */
#define __NR_getcwd          17
#define __NR_mkdirat         34
#define __NR_unlinkat        35
#define __NR_linkat          37
#define __NR_renameat        38
#define __NR_chdir           49

/* File IO Operations */
#define __NR_ioctl           29
#define __NR_truncate        45
#define __NR_ftruncate       46
#define __NR_faccessat       48
#define __NR_openat          56
#define __NR_close           57
#define __NR_lseek           62
#define __NR_read            63
#define __NR_write           64
#define __NR_fstatat         79
#define __NR_fstat           80

/* Process, Time, & System Info */
#define __NR_exit            93
#define __NR_nanosleep       101
#define __NR_uname           160
#define __NR_gettimeofday    169
#define __NR_getpid          172
#define __NR_getuid          174
#define __NR_geteuid         175
#define __NR_getgid          176
#define __NR_getegid         177

/* Advanced File Operations */
#ifndef __NR_copy_file_range
#define __NR_copy_file_range 326
#endif

/* Legacy Syscalls (Expected by older code / explicit fallbacks) */
#define __NR_open            1024
#define __NR_link            1025
#define __NR_unlink          1026
#define __NR_mkdir           1030
#define __NR_rename          1034
#define __NR_stat            1038
#define __NR_lstat           1039
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indirect numbered system call wrapper.
 */
long syscall(long number, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_SYSCALL_H */
