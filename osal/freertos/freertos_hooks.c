/**
 * @file felix/osal/freertos/freertos_hooks.c
 * @brief RISC-V Exceptions and FreeRTOS Diagnostic Hooks
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>

extern void freertos_risc_v_trap_handler(void);


/* =========================================================================
 * FreeRTOS Diagnostic Hooks
 * ========================================================================= */

void vApplicationMallocFailedHook(void) {
    printf("\r\n[FreeRTOS PANIC] Out of Heap Memory!\r\n");
    fflush(stdout);
    __asm__ volatile("ebreak");
    while(1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("\r\n[FreeRTOS PANIC] Stack Overflow detected in task: %s\r\n", pcTaskName);
    fflush(stdout);
    __asm__ volatile("ebreak");
    while(1);
}

/* =========================================================================
 * RISC-V Specific Exception Catchers
 * ========================================================================= */

volatile uint64_t debug_mcause = 0;
volatile uint64_t debug_mie = 0;
volatile uint64_t debug_mip = 0;

extern void handle_m_ext_interrupt(void);
extern void handle_m_soft_interrupt(void);

void __attribute__((used)) freertos_risc_v_application_interrupt_handler(uintptr_t mcause) {
    uintptr_t cause_code = mcause & 0x7FFFFFFFFFFFFFFF;

    if (cause_code == 11) {
        handle_m_ext_interrupt();
    } else if (cause_code == 3) {
        handle_m_soft_interrupt();
    } else {
        __asm__ volatile("csrci mstatus, 8");
        __asm__ volatile("ebreak");
        while(1);
    }
}

void __attribute__((used)) freertos_risc_v_application_exception_handler(uintptr_t mcause) {
    __asm__ volatile("csrci mstatus, 8");

    debug_mcause = mcause;
    __asm__ volatile("csrr %0, mie" : "=r"(debug_mie));
    __asm__ volatile("csrr %0, mip" : "=r"(debug_mip));

    __asm__ volatile("ebreak");
    while(1);
}
