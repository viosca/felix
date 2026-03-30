/** * @file arch/riscv64/include/cpu.h
 * @brief RISC-V 64-bit Architecture CPU Operations.
 */

#ifndef ARCH_RISCV64_CPU_H
#define ARCH_RISCV64_CPU_H

/**
 * @brief Puts the CPU into a low-power idle state until an interrupt arrives.
 * Architecture: RISC-V 64-bit (Uses the 'wfi' instruction)
 * * @note Must be wrapped in a while-loop by callers to protect
 * against spurious wakeups.
 */
static inline void arch_cpu_idle(void) {
    __asm__ volatile ("wfi");
}

#endif /* ARCH_RISCV64_CPU_H */
