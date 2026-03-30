/**
 * @file mm.c
 * @brief RISC-V 64-bit physical memory allocation backends.
 */

#include "mm.h"

/* =========================================================================
 * Linker Script Boundaries
 * These symbols are injected by the GCC linker (e.g., mpfs-lim.ld).
 * ========================================================================= */

extern char __heap_start[];
extern char __heap_end[];

void *arch_sbrk(ptrdiff_t incr) {
    static char *heap_ptr = NULL;

    /* Initialize the heap pointer on the first call */
    if (heap_ptr == NULL) {
        heap_ptr = __heap_start;
    }

    char *prev_heap_ptr = heap_ptr;

    /* A 0 increment is just a request for the current heap boundary */
    if (incr == 0) {
        return (void *)prev_heap_ptr;
    }

    /* Hardware Performance: Align all memory allocations to 16-byte boundaries */
    ptrdiff_t aligned_incr = (incr + 15) & ~15;

    /* Safety Check: Prevent the heap from colliding with the stack/BSS */
    if (heap_ptr + aligned_incr > __heap_end || heap_ptr + aligned_incr < __heap_start) {
        return (void *)-1;
    }

    /* Advance the physical pointer and return the chunk */
    heap_ptr += aligned_incr;
    return (void *)prev_heap_ptr;
}
