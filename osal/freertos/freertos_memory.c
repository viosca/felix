/**
 * @file felix/osal/freertos/freertos_memory.c
 * @brief FreeRTOS Memory Allocation Backend for Felix OS
 */
#include "FreeRTOS.h"
#include "kernel/kmalloc.h"
#include <string.h> /* For memcpy */

/* Because FreeRTOS heap_4 does not support realloc(), we must track the
 * size of each allocation ourselves. We do this by over-allocating by
 * sizeof(size_t) and hiding the size just before the returned pointer. */

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    size_t *ptr = pvPortMalloc(size + sizeof(size_t));
    if (!ptr) return NULL;

    *ptr = size;
    return (void *)(ptr + 1);
}

void kfree(void *ptr) {
    if (!ptr) return;

    size_t *real_ptr = (size_t *)ptr - 1;
    vPortFree(real_ptr);
}

void *krealloc(void *ptr, size_t size) {
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return kmalloc(size);
    }

    size_t old_size = *((size_t *)ptr - 1);
    void *new_ptr = kmalloc(size);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);

    return new_ptr;
}
