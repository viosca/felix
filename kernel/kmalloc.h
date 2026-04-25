/**
 * @file kmalloc.h
 * @brief Kernel Memory Allocation Subsystem.
 */

#ifndef FELIX_KERNEL_KMALLOC_H
#define FELIX_KERNEL_KMALLOC_H

#include <stddef.h>

/**
 * @brief Allocate raw memory in kernel space.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL if out of memory.
 */
void *kmalloc(size_t size);

/**
 * @brief Free memory previously allocated by flx_kmalloc.
 * @param ptr Pointer to the memory block.
 */
void kfree(void *ptr);

/**
 * @brief Allocate zero-initialized memory in kernel space.
 * Equivalent to kmalloc() followed by memset(0).
 * @param size Number of bytes to allocate.
 * @return Pointer to zeroed memory, or NULL if out of memory.
 */
void *kzalloc(size_t size);


/**
 * @brief Duplicate a string in the kernel heap.
 * @param The string to duplicate.
 * @return Pointer to the duplicated string.
 */
char *kstrdup(const char *s);

#endif /* FELIX_KERNEL_KMALLOC_H */
