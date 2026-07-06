/**
 * @file kmalloc.c
 * @brief Kernel Memory Allocation Subsystem Implementation.
 */

#include "kernel/kmalloc.h"
#include <string.h> /* For memset */

/* The active OSAL Memory Backend Engine.
 * By removing 'static' and making it 'weak', the OSAL backend can
 * statically inject its engine pointer at link-time! */
__attribute__((weak)) const kmem_operations_t *g_kmem_ops = NULL;

void kmalloc_register_backend(const kmem_operations_t *ops) {
    g_kmem_ops = ops;
}

void *kmalloc(size_t size) {
    if (!g_kmem_ops || !g_kmem_ops->alloc || size == 0) return NULL;
    return g_kmem_ops->alloc(size);
}

void kfree(void *ptr) {
    if (!g_kmem_ops || !g_kmem_ops->free || !ptr) return;
    g_kmem_ops->free(ptr);
}

void *krealloc(void *ptr, size_t size) {
    if (!g_kmem_ops || !g_kmem_ops->realloc) return NULL;
    return g_kmem_ops->realloc(ptr, size);
}

/* =========================================================================
 * Kernel Utility Allocators
 * ========================================================================= */

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

char *kstrdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = kmalloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}
