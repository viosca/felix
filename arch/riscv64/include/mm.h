/**
 * @file arch/riscv64/include/mm.h
 * @brief RISC-V 64-bit Architecture Memory Management.
 * * Defines the low-level physical memory hooks required by the OS kernel.
 */

#ifndef ARCH_RISCV64_MM_H
#define ARCH_RISCV64_MM_H

#include <stddef.h>

/**
 * @brief Extends the physical heap boundary.
 * * Bumps the physical memory allocator boundary between the linker-defined
 * __heap_start and __heap_end sections.
 * * @param incr The number of bytes to increment the heap by.
 * @return A pointer to the start of the newly allocated memory, or
 * (void *)-1 if the heap bounds are exceeded.
 */
void *arch_sbrk(ptrdiff_t incr);

#endif /* ARCH_RISCV64_MM_H */
