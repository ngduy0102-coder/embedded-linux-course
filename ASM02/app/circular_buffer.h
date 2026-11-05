#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "sms_common.h"

/* Circular buffer status codes */
typedef enum {
    CB_OK = 0,
    CB_FULL,
    CB_CLOSED,
    CB_ERROR
} CBStatus;

/* Circular buffer structure */
typedef struct {
    SensorData *buffer;

    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;

    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;

    bool closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} CircularBuffer;

/* Initialize circular buffer */
int cb_init(CircularBuffer *cb, size_t capacity);

/* Insert a sensor sample */
CBStatus cb_push(CircularBuffer *cb, const SensorData *data);

/* Retrieve a sensor sample */
CBStatus cb_pop(CircularBuffer *cb, SensorData *data);

/* Retrieve buffer statistics */
int cb_get_stats(CircularBuffer *cb, BufferStats *stats);

/* Close buffer and wake waiting threads */
int cb_close(CircularBuffer *cb);

/* Release buffer resources */
void cb_destroy(CircularBuffer *cb);

#endif /* CIRCULAR_BUFFER_H */
