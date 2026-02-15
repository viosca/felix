#include "ring_buffer.h"

void felix_ringbuffer_init(FelixRingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

bool felix_ringbuffer_push(FelixRingBuffer_t *rb, uint8_t data) {
    uint32_t next = (rb->head + 1) % FELIX_RINGBUFFER_SIZE;

    // Check if full (next write would overwrite tail)
    if (next == rb->tail) {
        return false;
    }

    rb->buffer[rb->head] = data;
    rb->head = next;
    return true;
}

bool felix_ringbuffer_pop(FelixRingBuffer_t *rb, uint8_t *data) {
    // Check if empty
    if (rb->head == rb->tail) {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % FELIX_RINGBUFFER_SIZE;
    return true;
}

bool felix_ringbuffer_is_empty(FelixRingBuffer_t *rb) {
    return (rb->head == rb->tail);
}

bool felix_ringbuffer_is_full(FelixRingBuffer_t *rb) {
    return ((rb->head + 1) % FELIX_RINGBUFFER_SIZE) == rb->tail;
}

bool felix_ringbuffer_unpush(FelixRingBuffer_t *rb) {
    if (felix_ringbuffer_is_empty(rb)) return false;

    // Move head back one position (handling wrap-around)
    // Note: This is safe in ISR (Producer) because Consumer only touches tail.
    if (rb->head == 0) {
        rb->head = FELIX_RINGBUFFER_SIZE - 1;
    } else {
        rb->head--;
    }
    return true;
}
