#ifndef FELIX_KERNEL_RING_BUFFER_H
#define FELIX_KERNEL_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Define the size of the buffer.
 * Must be a power of 2 for efficient wrapping (e.g., 64, 128, 256).
 */
#define RING_BUFFER_SIZE 128

typedef struct {
    volatile uint8_t buffer[RING_BUFFER_SIZE];
    volatile uint32_t head; // Producer writes here
    volatile uint32_t tail; // Consumer reads here
} ring_buffer_t;

/* Initialize the ring buffer */
void rb_init(ring_buffer_t *rb);

/* Push a byte into the buffer (called by ISR)
 * Returns true if successful, false if buffer is full.
 */
bool rb_push(ring_buffer_t *rb, uint8_t data);

/* Unpush a byte from the buffer (called by isr)
 * Returns true if successful, false if buffer is empty.
 * Places data in *data pointer.
 */
bool rb_unpush(ring_buffer_t *rb);

/* Pop a byte from the buffer (called by _read)
 * Returns true if successful, false if buffer is empty.
 * Places data in *data pointer.
 */
bool rb_pop(ring_buffer_t *rb, uint8_t *data);

/* Check if buffer is empty */
bool rb_is_empty(ring_buffer_t *rb);

/* Check if buffer is full */
bool rb_is_full(ring_buffer_t *rb);

#endif // FELIX_KERNEL_RING_BUFFER_H
