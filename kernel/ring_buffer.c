/**
 * @file ring_buffer.c
 * @brief Implementation of the IRQ-safe ring buffer.
 */

#include "kernel/ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb)
{
    if (!rb)
        return;

    rb->head = 0;
    rb->tail = 0;
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    if (!rb)
        return true;

    return (rb->head == rb->tail);
}

bool ring_buffer_is_full(ring_buffer_t *rb)
{
    if (!rb)
        return true;

    /* Next head position, wrapping using bitwise AND */
    size_t next_head = (rb->head + 1) & (RING_BUFFER_SIZE - 1);

    return (next_head == rb->tail);
}

int ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    size_t next_head;

    if (!rb)
        return -1;

    next_head = (rb->head + 1) & (RING_BUFFER_SIZE - 1);

    if (next_head == rb->tail) {
        /* Buffer is full, drop the character */
        return -1;
    }

    rb->buffer[rb->head] = data;

    /* Memory barrier goes here if running on a multicore CPU,
     * but for single-core baremetal with volatile, this is safe.
     */
    rb->head = next_head;

    return 0;
}

int ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    if (!rb || !data)
        return -1;

    if (rb->head == rb->tail) {
        /* Buffer is empty */
        return -1;
    }

    *data = rb->buffer[rb->tail];

    /* Update tail pointer after reading the data, wrapping using bitwise AND */
    rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);

    return 0;
}
