/**
 * @file kmalloc.h
 * @brief Kernel Memory Allocation Subsystem.
 */

#ifndef _FLX_KERNEL_KMALLOC_H
#define _FLX_KERNEL_KMALLOC_H

#include <stddef.h>

/**
 * @brief OSAL Memory Operations structure.
 * The active RTOS backend must register these during early boot.
 */
typedef struct {
    void *(*alloc)(size_t size);
    void  (*free)(void *ptr);
    void *(*realloc)(void *ptr, size_t size);
} kmem_operations_t;

/**
 * @brief Register the memory allocation backend.
 * Must be called before ANY dynamic allocation occurs.
 */
void kmalloc_register_backend(const kmem_operations_t *ops);

/**
 * @brief Allocate raw memory in kernel space.
 */
void *kmalloc(size_t size);

/**
 * @brief Free memory previously allocated by kmalloc.
 */
void kfree(void *ptr);

/**
 * @brief Reallocate memory in kernel space.
 */
void *krealloc(void *ptr, size_t size);

/**
 * @brief Allocate zero-initialized memory in kernel space.
 */
void *kzalloc(size_t size);

/**
 * @brief Duplicate a string in the kernel heap.
 */
char *kstrdup(const char *s);

#endif /* _FLX_KERNEL_KMALLOC_H */
