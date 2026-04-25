/* @file felix/arch/riscv64/timer.c
 *
 */
#include "arch/riscv64/include/timer.h"
#include <stdint.h>

/* Standard RISC-V CLINT Memory Map */
#ifndef RISCV_CLINT_MTIME_ADDR
#define RISCV_CLINT_MTIME_ADDR 0x0200BFF8ULL
#endif

/* Microchip PolarFire SoC typically runs the MTIME counter at 1 MHz,
 * whereas QEMU defaults to 10 MHz. */
#ifndef RISCV_TIMER_FREQ_HZ
#define RISCV_TIMER_FREQ_HZ 1000000
#endif

uint64_t arch_timer_get_ticks(void) {
    /* The peripheral bus is likely 32-bits wide. We must read the
     * 64-bit register in two 32-bit chunks to prevent "Torn Reads". */
    volatile uint32_t *mtime_low  = (volatile uint32_t *)(RISCV_CLINT_MTIME_ADDR);
    volatile uint32_t *mtime_high = (volatile uint32_t *)(RISCV_CLINT_MTIME_ADDR + 4);

    uint32_t high, low;
    do {
        high = *mtime_high;
        low  = *mtime_low;
    } while (high != *mtime_high); /* If high changed, we caught it mid-tick! Try again. */

    return ((uint64_t)high << 32) | low;
}

uint64_t arch_timer_get_ms(void) {
    uint64_t ticks = arch_timer_get_ticks();
    /* Convert raw hardware ticks to milliseconds */
    return ticks / (RISCV_TIMER_FREQ_HZ / 1000);
}

uint64_t arch_timer_get_us(void) {
    uint64_t ticks = arch_timer_get_ticks();
    /* Convert raw hardware ticks to microseconds safely */
    return ticks / (RISCV_TIMER_FREQ_HZ / 1000000);
}
