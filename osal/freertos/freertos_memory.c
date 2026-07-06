/**
 * @file felix/osal/freertos/freertos_memory.c
 * @brief FreeRTOS Memory Allocation Backend for Felix OS
 */
#include "FreeRTOS.h"
#include "kernel/kmalloc.h"
#include <string.h> /* For memcpy */

/* RISC-V 64-bit STRICTLY requires 16-byte alignment for many C-library
 * (newlib) optimizations like vector or floating-point memory clears.
 * If we offset the FreeRTOS pointer by 8 bytes, we break that alignment
 * and trigger a hardware Load/Store Misaligned Exception! */
#define FLX_MALLOC_ALIGN 16

static void *freertos_alloc(size_t size) {
    if (size == 0) return NULL;

    /* To strictly guarantee 16-byte alignment regardless of FreeRTOS settings,
     * we must overallocate by enough to shift the pointer ourselves.
     * 16 bytes for alignment + 16 bytes for our header/padding = 32 bytes extra. */
    void *raw_ptr = pvPortMalloc(size + 32);
    if (!raw_ptr) return NULL;

    /* Calculate aligned address that leaves at least 16 bytes of space before it */
    uintptr_t addr = (uintptr_t)raw_ptr + 16;
    addr = (addr + 15) & ~15; /* Round up to nearest multiple of 16 */

    uint8_t *aligned_ptr = (uint8_t *)addr;

    /* Store the original raw pointer AND the size just behind the aligned pointer */
    size_t *meta = (size_t *)aligned_ptr;
    meta[-1] = size;
    meta[-2] = (size_t)raw_ptr;

    return aligned_ptr;
}

static void freertos_free(void *ptr) {
    if (!ptr) return;

    /* Recover the original raw pointer from our hidden metadata */
    size_t *meta = (size_t *)ptr;
    void *raw_ptr = (void *)meta[-2];

    vPortFree(raw_ptr);
}

static void *freertos_realloc(void *ptr, size_t size) {
    if (size == 0) {
        freertos_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return freertos_alloc(size);
    }

    /* Recover the old size from our hidden metadata */
    size_t *meta = (size_t *)ptr;
    size_t old_size = meta[-1];

    void *new_ptr = freertos_alloc(size);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);

    freertos_free(ptr);
    return new_ptr;
}

/* =========================================================================
 * The Engine Export
 * ========================================================================= */

const kmem_operations_t freertos_memory_engine = {
    .alloc   = freertos_alloc,
    .free    = freertos_free,
    .realloc = freertos_realloc
};

const kmem_operations_t * const g_kmem_ops = &freertos_memory_engine;
