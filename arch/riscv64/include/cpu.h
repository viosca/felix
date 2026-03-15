#ifndef FLX_HAL_CPU_H
#define FLX_HAL_CPU_H

/**
 * @brief Puts the CPU into a low-power state until an interrupt arrives.
 * * Architecture: RISC-V 64-bit
 * Note: Must be wrapped in a while-loop by callers to protect
 * against spurious wakeups.
 */
static inline void flx_cpu_wait_for_interrupt(void) {
    __asm__ volatile ("wfi");
}

#endif /* FLX_HAL_CPU_H */
