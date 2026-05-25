/**
 * @file felix/kernel/malloc_wrappers.c
 * @brief Reroutes standard Newlib C-library memory allocation into the OS heap.
 */

#include <stddef.h>
#include <string.h>
#include <reent.h> /* For Newlib's struct _reent */
#include "kernel/kmalloc.h"

/* =========================================================================
 * GNU Linker Wrapper Functions (Standard)
 * ========================================================================= */

void *__wrap_malloc(size_t size) {
    return kmalloc(size);
}

void __wrap_free(void *ptr) {
    kfree(ptr);
}

void *__wrap_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *__wrap_realloc(void *ptr, size_t size) {
    return krealloc(ptr, size);
}

/* =========================================================================
 * Newlib Reentrant (_r) Wrapper Functions
 * Standard C-library internals (like strdup, vasprintf, fopen) bypass
 * normal malloc and call these directly. We MUST intercept them too!
 * ========================================================================= */

void *__wrap__malloc_r(struct _reent *r, size_t size) {
    (void)r; /* Thread-safety is handled by FreeRTOS internally */
    return kmalloc(size);
}

void __wrap__free_r(struct _reent *r, void *ptr) {
    (void)r;
    kfree(ptr);
}

void *__wrap__calloc_r(struct _reent *r, size_t nmemb, size_t size) {
    (void)r;
    return __wrap_calloc(nmemb, size);
}

void *__wrap__realloc_r(struct _reent *r, void *ptr, size_t size) {
    (void)r;
    return krealloc(ptr, size);
}
