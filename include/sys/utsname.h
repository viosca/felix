/**
 * @file utsname.h
 * @brief POSIX System Information Structure
 */

#ifndef _FLX_INCLUDE_SYS_UTSNAME_H
#define _FLX_INCLUDE_SYS_UTSNAME_H

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX standard typically uses 65 bytes (64 chars + null terminator) */
#define _UTSNAME_LENGTH 65

struct utsname {
    char sysname[_UTSNAME_LENGTH];  /* Name of the operating system (e.g., "Felix") */
    char nodename[_UTSNAME_LENGTH]; /* Name of the node on the network */
    char release[_UTSNAME_LENGTH];  /* Current release level of the OS */
    char version[_UTSNAME_LENGTH];  /* Current version level of the OS */
    char machine[_UTSNAME_LENGTH];  /* Hardware identifier (e.g., "riscv64") */
};

/* Standard POSIX function prototype */
int uname(struct utsname *name);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_UTSNAME_H */
