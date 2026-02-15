#ifndef FELIX_KERNEL_RING_BUFFER_H
#define FELIX_KERNEL_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Define the size of the buffer.
 * Must be a power of 2 for efficient wrapping (e.g., 64, 128, 256).
 */
#define FELIX_RINGBUFFER_SIZE 128

typedef struct {
    volatile uint8_t buffer[FELIX_RINGBUFFER_SIZE];
    volatile uint32_t head; // Producer writes here
    volatile uint32_t tail; // Consumer reads here
} FelixRingBuffer_t;

/* Initialize the ring buffer */
void felix_ringbuffer_init(FelixRingBuffer_t *rb);

/* Push a byte into the buffer (called by ISR)
 * Returns true if successful, false if buffer is full.
 */
bool felix_ringbuffer_push(FelixRingBuffer_t *rb, uint8_t data);

/* Unpush a byte from the buffer (called by isr)
 * Returns true if successful, false if buffer is empty.
 * Places data in *data pointer.
 */
bool felix_ringbuffer_unpush(FelixRingBuffer_t *rb);

/* Pop a byte from the buffer (called by _read)
 * Returns true if successful, false if buffer is empty.
 * Places data in *data pointer.
 */
bool felix_ringbuffer_pop(FelixRingBuffer_t *rb, uint8_t *data);

/* Check if buffer is empty */
bool felix_ringbuffer_is_empty(FelixRingBuffer_t *rb);

/* Check if buffer is full */
bool felix_ringbuffer_is_full(FelixRingBuffer_t *rb);

#endif // FELIX_KERNEL_RING_BUFFER_H
