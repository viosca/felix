/**
 * @file ring_buffer.h
 * @brief A lightweight, IRQ-safe 128-byte ring buffer.
 */

#ifndef _FLX_RING_BUFFER_H
#define _FLX_RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ring_buffer.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Must be a power of 2 for optimal wrapping, 128 as requested */
#define RING_BUFFER_SIZE 128

typedef struct ring_buffer {
    uint8_t buffer[RING_BUFFER_SIZE];

    /* * Volatile ensures the compiler doesn't optimize away reads/writes
     * to these indices when they are modified inside an ISR.
     */
    volatile size_t head; /* Write index */
    volatile size_t tail; /* Read index */
} ring_buffer_t;


/**
 * @brief Initializes or resets a ring buffer.
 */
void ring_buffer_init(ring_buffer_t *rb);

/**
 * @brief Checks if the ring buffer is empty.
 * @return true if empty, false otherwise.
 */
bool ring_buffer_is_empty(ring_buffer_t *rb);

/**
 * @brief Checks if the ring buffer is full.
 * @return true if full, false otherwise.
 */
bool ring_buffer_is_full(ring_buffer_t *rb);

/**
 * @brief Writes a single byte into the ring buffer.
 * @return 0 on success, -1 if the buffer is full.
 */
int ring_buffer_write(ring_buffer_t *rb, uint8_t data);

/**
 * @brief Reads a single byte from the ring buffer.
 * @return 0 on success, -1 if the buffer is empty.
 */
int ring_buffer_read(ring_buffer_t *rb, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_RING_BUFFER_H */
