/**
 * @file arch/riscv64/include/timer.h
 * @brief RISC-V 64-bit Architecture Timer Hook.
 */

#ifndef ARCH_RISCV64_TIMER_H
#define ARCH_RISCV64_TIMER_H

#include <stdint.h>

/**
 * @brief Retrieves the monotonic hardware uptime in ticks.
 * @return Ticks elapsed since the hardware was powered on.
 */
uint64_t arch_timer_get_ticks(void);

/**
 * @brief Retrieves the monotonic hardware uptime in ms.
 * @return Milliseconds elapsed since the hardware was powered on.
 */
uint64_t arch_timer_get_ms(void);

/**
 * @brief Retrieves the monotonic hardware uptime in us.
 * @return Microseconds elapsed since the hardware was powered on.
 */
uint64_t arch_timer_get_us(void);


#endif /* ARCH_RISCV64_TIMER_H */
