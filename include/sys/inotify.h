/**
 * @file felix/include/sys/inotify.h
 * @brief POSIX/Linux inotify extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_INOTIFY_H
#define _FLX_INCLUDE_SYS_INOTIFY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * inotify Event Structure
 * ========================================================================= */
struct inotify_event {
    int      wd;       /* Watch descriptor */
    uint32_t mask;     /* Mask describing event */
    uint32_t cookie;   /* Unique cookie associating related events */
    uint32_t len;      /* Size of name field */
    char     name[];   /* Optional null-terminated name */
};

/* =========================================================================
 * inotify Event Masks
 * ========================================================================= */
#define IN_ACCESS        0x00000001 /* File was accessed */
#define IN_MODIFY        0x00000002 /* File was modified */
#define IN_ATTRIB        0x00000004 /* Metadata changed */
#define IN_CLOSE_WRITE   0x00000008 /* Writtable file was closed */
#define IN_CLOSE_NOWRITE 0x00000010 /* Unwrittable file closed */
#define IN_OPEN          0x00000020 /* File was opened */
#define IN_MOVED_FROM    0x00000040 /* File was moved from X */
#define IN_MOVED_TO      0x00000080 /* File was moved to Y */
#define IN_CREATE        0x00000100 /* Subfile was created */
#define IN_DELETE        0x00000200 /* Subfile was deleted */
#define IN_DELETE_SELF   0x00000400 /* Self was deleted */
#define IN_MOVE_SELF     0x00000800 /* Self was moved */

/* Events sent by the kernel */
#define IN_UNMOUNT       0x00002000 /* Backing fs was unmounted */
#define IN_Q_OVERFLOW    0x00004000 /* Event queued overflowed */
#define IN_IGNORED       0x00008000 /* File was ignored */

/* Special flags */
#define IN_ONLYDIR       0x01000000 /* Only watch the path if it is a directory */
#define IN_DONT_FOLLOW   0x02000000 /* Do not follow a sym link */
#define IN_EXCL_UNLINK   0x04000000 /* Exclude events on unlinked objects */
#define IN_MASK_ADD      0x20000000 /* Add to the mask of an already existing watch */
#define IN_ISDIR         0x40000000 /* Event occurred against dir */
#define IN_ONESHOT       0x80000000 /* Only send event once */

/* Init flags */
#define IN_NONBLOCK      00004000
#define IN_CLOEXEC       02000000

/* =========================================================================
 * Function Prototypes
 * ========================================================================= */
int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_INOTIFY_H */
