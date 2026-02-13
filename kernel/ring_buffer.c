#include "ring_buffer.h"

void rb_init(ring_buffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

bool rb_push(ring_buffer_t *rb, uint8_t data) {
    uint32_t next = (rb->head + 1) % RING_BUFFER_SIZE;

    // Check if full (next write would overwrite tail)
    if (next == rb->tail) {
        return false;
    }

    rb->buffer[rb->head] = data;
    rb->head = next;
    return true;
}

bool rb_pop(ring_buffer_t *rb, uint8_t *data) {
    // Check if empty
    if (rb->head == rb->tail) {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    return true;
}

bool rb_is_empty(ring_buffer_t *rb) {
    return (rb->head == rb->tail);
}

bool rb_is_full(ring_buffer_t *rb) {
    return ((rb->head + 1) % RING_BUFFER_SIZE) == rb->tail;
}

bool rb_unpush(ring_buffer_t *rb) {
    if (rb_is_empty(rb)) return false;

    // Move head back one position (handling wrap-around)
    // Note: This is safe in ISR (Producer) because Consumer only touches tail.
    if (rb->head == 0) {
        rb->head = RING_BUFFER_SIZE - 1;
    } else {
        rb->head--;
    }
    return true;
}
