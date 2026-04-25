/**
 * @file kmalloc.c
 * @brief Kernel Memory Allocation Subsystem Implementation.
 */

#include "kernel/kmalloc.h"
#include <stdlib.h> /* For underlying malloc/free (temporary) */
#include <string.h> /* For memset */

/* TODO: Eventually replace stdlib malloc with a true kernel heap
 * (e.g., a slab allocator or buddy allocator) tied directly to
 * the physical page allocator. */

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Temporary wrapper around standard library malloc */
    void *ptr = malloc(size);

    /* TODO: Add memory tracking/leak detection metrics here later! */

    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    /* Temporary wrapper around standard library free */
    free(ptr);
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

char *kstrdup(const char *s) {
    if (!s) return NULL;

    char *copy = kmalloc(strlen(s) + 1);
    if (copy) {
        strcpy(copy, s);
    }

    return copy;
}
