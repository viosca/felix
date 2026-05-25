/**
 * @file felix/osal/freertos/freertos_sched.c
 * @brief FreeRTOS Implementation of the Scheduler Abstraction Layer (SAL)
 */

#include "kernel/sched.h"

/* The Quarantine Boundary:
 * These headers stay strictly inside this C file! */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

extern void freertos_risc_v_trap_handler(void);

/* =========================================================================
 * FreeRTOS Wrapper Functions
 * ========================================================================= */

static void freertos_critical_enter(void) {
    taskENTER_CRITICAL();
}

static void freertos_critical_exit(void) {
    taskEXIT_CRITICAL();
}

static int freertos_init(void) {
    /* FreeRTOS doesn't require explicit initialization before task creation.
     * Hardware timers are set up during vTaskStartScheduler. */
    return 0;
}

//static int freertos_thread_create(void **handle, const char *name, size_t stack_bytes, sched_thread_func_t start_func, void *arg)
static int freertos_thread_create(void **handle, const char *name, size_t stack_bytes, int priority, sched_thread_func_t start_func, void *arg)
{
    TaskHandle_t rtos_handle = NULL;

    /* CRITICAL TRANSLATION:
     * FreeRTOS expects stack size in WORDS (StackType_t), not BYTES!
     * On RISC-V 64-bit (PolarFire), a word is 8 bytes.
     * If ToyBox asks for 8192 bytes, we must ask FreeRTOS for 1024 words. */
    configSTACK_DEPTH_TYPE stack_words = stack_bytes / sizeof(StackType_t);

    /* We map Felix's generic priority directly to FreeRTOS.
     * Ensure the requested priority doesn't exceed configMAX_PRIORITIES - 1 */
    if (priority >= configMAX_PRIORITIES) {
        priority = configMAX_PRIORITIES - 1;
    }
    if (priority < tskIDLE_PRIORITY) {
        priority = tskIDLE_PRIORITY;
    }

    /* We map Felix's generic function pointer directly to FreeRTOS's TaskFunction_t.
    * Defaulting to priority 1 (just above idle). */
    BaseType_t res = xTaskCreate(
        (TaskFunction_t)start_func,
        name,
        stack_words,
        arg,
        priority,
        &rtos_handle
    );

    if (res != pdPASS) {
        return -1; /* ENOMEM (Usually means FreeRTOS heap is full) */
    }

    /* Cast the FreeRTOS handle to an opaque void pointer for Felix's PCB */
    if (handle) {
        *handle = (void *)rtos_handle;
    }

    return 0;
}

static void freertos_thread_exit(void) {
    /* Passing NULL tells FreeRTOS to delete the currently running task */
    vTaskDelete(NULL);
}

static void freertos_yield(void) {
    taskYIELD();
}

static void freertos_sleep_ms(uint32_t ms) {
    /* Translate standard milliseconds into FreeRTOS hardware ticks */
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void freertos_start(void) {

    /* HIJACK THE TRAP VECTOR:
         * Tell the RISC-V CPU to route all future interrupts (including the
         * MTIME timer) to the FreeRTOS assembly handler instead of the HAL. */
        __asm__ volatile("csrw mtvec, %0" :: "r"(freertos_risc_v_trap_handler));

    /* This configures the RISC-V machine timer (SysTick) and starts preemption.
     * If this function returns, it means the FreeRTOS heap had 0 bytes left. */
    vTaskStartScheduler();

    /* We should never reach here! */
    while(1);
}

static int freertos_sem_create(void **handle) {
    *handle = (void *)xSemaphoreCreateBinary();
    return (*handle != NULL) ? 0 : -1;
}

static void freertos_sem_take(void *handle, uint32_t timeout_ms) {
    TickType_t ticks;

    if (timeout_ms == SCHED_WAIT_FOREVER) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }

    xSemaphoreTake((SemaphoreHandle_t)handle, ticks);
}

static void freertos_sem_give_from_isr(void *handle) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void freertos_set_tls(void *ptr) {
    /* NULL means "the currently running task".
     * 0 is the index in the TLS array we enabled in the config. */
    vTaskSetThreadLocalStoragePointer(NULL, 0, ptr);
}

static void *freertos_get_tls(void) {
    return pvTaskGetThreadLocalStoragePointer(NULL, 0);
}

/* =========================================================================
 * The Engine Export
 * ========================================================================= */

/* We pack the wrappers into the struct defined by our contract */
static const struct sched_operations freertos_engine = {
    .init              = freertos_init,
    .thread_create     = freertos_thread_create,
    .thread_exit       = freertos_thread_exit,
    .yield             = freertos_yield,
    .sleep_ms          = freertos_sleep_ms,
    .sem_create        = freertos_sem_create,
    .sem_give_from_isr = freertos_sem_give_from_isr,
    .sem_take          = freertos_sem_take,

    .critical_enter    = freertos_critical_enter,
    .critical_exit     = freertos_critical_exit,

    .set_tls           = freertos_set_tls,
    .get_tls           = freertos_get_tls,

    .start             = freertos_start
};

/* We define the global pointer that Felix will use to drive the system.
 * By pointing it here, FreeRTOS is now officially the active engine. */
const struct sched_operations *active_scheduler = &freertos_engine;
