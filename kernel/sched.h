/**
 * @file sched.h
 * @brief Scheduler Abstraction Layer (SAL)
 * * This defines the hardware-agnostic contract that any underlying
 * operating system (FreeRTOS, Zephyr, bare-metal loop) must fulfill
 * to act as the engine for Felix.
 */

#ifndef _FLX_KERNEL_SCHED_H
#define _FLX_KERNEL_SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Defines the maximum possible timeout for blocking OS calls */
#define SCHED_WAIT_FOREVER 0xFFFFFFFF

/**
 * @brief Function prototype for a standard thread entry point.
 */
typedef void (*sched_thread_func_t)(void *arg);

/**
 * @brief The Scheduler Operations Table
 * * This is the VFS-style routing table for execution. The active
 * RTOS plugin must populate this struct with its specific functions.
 */
struct sched_operations {
    /**
     * @brief Initialize the scheduler hardware and internal state.
     * @return 0 on success, negative error code on failure.
     */
    int (*init)(void);

    /**
     * @brief Create a new thread of execution.
     * @param handle Pointer to store the newly created opaque task handle.
     * @param name A string name for the task (useful for debugging).
     * @param stack_size The size of the stack to allocate in bytes.
     * @param start_func The C function the thread will begin executing.
     * @param arg The argument passed into the start_func.
     * @return 0 on success, negative error code on failure.
     */
    int (*thread_create)(void **handle, const char *name, size_t stack_size, int priority, sched_thread_func_t start_func, void *arg);

    /**
     * @brief Terminate the currently running thread.
     */
    void (*thread_exit)(void);

    /**
     * @brief Voluntarily surrender the remainder of the CPU time slice.
     */
    void (*yield)(void);

    /**
     * @brief Put the currently running thread to sleep.
     * @param ms The number of milliseconds to sleep.
     */
    void (*sleep_ms)(uint32_t ms);

    /**
     * @brief Start the scheduler.
     * @note This function should configure the hardware timer and NEVER return.
     */
    void (*start)(void);

    /* Inside struct sched_operations */
    int (*sem_create)(void **handle);
    void (*sem_take)(void *handle, uint32_t timeout_ms);
    void (*sem_give_from_isr)(void *handle);

    /* Thread Local Storage (TLS) */
    void (*set_tls)(void *ptr);
    void *(*get_tls)(void);

    void (*critical_enter)(void);
    void (*critical_exit)(void);
};

/* The global pointer to the currently loaded engine */
extern const struct sched_operations *active_scheduler;

/* =========================================================================
 * Inline Convenience Wrappers
 * =========================================================================
 * These allow your process.c to cleanly call sched_yield() instead of
 * constantly typing active_scheduler->yield().
 */
static inline int sched_thread_create(void **handle, const char *name, size_t stack_size, int priority, sched_thread_func_t start_func, void *arg)
{
    if (active_scheduler && active_scheduler->thread_create)
        return active_scheduler->thread_create(handle, name, stack_size, priority, start_func, arg);
    return -1;
}

static inline void sched_thread_exit(void) {
    if (active_scheduler && active_scheduler->thread_exit)
        active_scheduler->thread_exit();
}

static inline void sched_yield(void) {
    if (active_scheduler && active_scheduler->yield)
        active_scheduler->yield();
}

static inline void sched_sleep_ms(uint32_t ms) {
    if (active_scheduler && active_scheduler->sleep_ms)
        active_scheduler->sleep_ms(ms);
}

/* =========================================================================
 * Semaphore / Synchronization Wrappers
 * ========================================================================= */

static inline int sched_sem_create(void **handle) {
    if (active_scheduler && active_scheduler->sem_create) {
        return active_scheduler->sem_create(handle);
    }
    return -1;
}

static inline void sched_sem_take(void *handle, uint32_t timeout_ms) {
    if (active_scheduler && active_scheduler->sem_take) {
        active_scheduler->sem_take(handle, timeout_ms);
    }
}

static inline void sched_sem_give_from_isr(void *handle) {
    if (active_scheduler && active_scheduler->sem_give_from_isr) {
        active_scheduler->sem_give_from_isr(handle);
    }
}

static inline void sched_set_tls(void *ptr) {
    if (active_scheduler && active_scheduler->set_tls)
        active_scheduler->set_tls(ptr);
}

static inline void *sched_get_tls(void) {
    if (active_scheduler && active_scheduler->get_tls)
        return active_scheduler->get_tls();
    return NULL;
}

static inline void sched_critical_enter(void) {
    if (active_scheduler && active_scheduler->critical_enter)
        active_scheduler->critical_enter();
}

static inline void sched_critical_exit(void) {
    if (active_scheduler && active_scheduler->critical_exit)
        active_scheduler->critical_exit();
}
#endif // _FLX_KERNEL_SCHED_H
