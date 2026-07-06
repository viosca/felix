/**
 * @file felix/include/sys/mman.h
 * @brief Memory Management shim for Toybox.
 */
#ifndef _FLX_INCLUDE_SYS_MMAN_H
#define _FLX_INCLUDE_SYS_MMAN_H

#include <sys/types.h>

/* Protection flags */
#define PROT_NONE       0x00
#define PROT_READ       0x01
#define PROT_WRITE      0x02
#define PROT_EXEC       0x04

/* Mapping flags */
#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_FAILED      ((void *)-1)

#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_MMAN_H */
