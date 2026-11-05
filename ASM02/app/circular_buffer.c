#include "circular_buffer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Initialize circular buffer */
int cb_init(CircularBuffer *cb, size_t capacity)
{
    int ret;

    if (cb == NULL || capacity == 0)
        return EINVAL;

    memset(cb, 0, sizeof(*cb));

    cb->buffer = calloc(capacity, sizeof(SensorData));
    if (cb->buffer == NULL)
        return ENOMEM;

    cb->capacity = capacity;

    ret = pthread_mutex_init(&cb->mutex, NULL);
    if (ret != 0) {
        free(cb->buffer);
        cb->buffer = NULL;
        return ret;
    }

    ret = pthread_cond_init(&cb->not_empty, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&cb->mutex);
        free(cb->buffer);
        cb->buffer = NULL;
        return ret;
    }

    return 0;
}

/* Insert a sensor sample */
CBStatus cb_push(CircularBuffer *cb, const SensorData *data)
{
    if (cb == NULL || data == NULL || cb->buffer == NULL)
        return CB_ERROR;

    pthread_mutex_lock(&cb->mutex);

    if (cb->closed) {
        pthread_mutex_unlock(&cb->mutex);
        return CB_CLOSED;
    }

    if (cb->size == cb->capacity) {
        cb->overflow_count++;
        pthread_mutex_unlock(&cb->mutex);
        return CB_FULL;
    }

    cb->buffer[cb->head] = *data;
    cb->head = (cb->head + 1) % cb->capacity;

    cb->size++;
    cb->total_produced++;

    pthread_cond_signal(&cb->not_empty);
    pthread_mutex_unlock(&cb->mutex);

    return CB_OK;
}

/* Retrieve a sensor sample */
CBStatus cb_pop(CircularBuffer *cb, SensorData *data)
{
    if (cb == NULL || data == NULL || cb->buffer == NULL)
        return CB_ERROR;

    pthread_mutex_lock(&cb->mutex);

    while (cb->size == 0 && !cb->closed)
        pthread_cond_wait(&cb->not_empty, &cb->mutex);

    if (cb->size == 0 && cb->closed) {
        pthread_mutex_unlock(&cb->mutex);
        return CB_CLOSED;
    }

    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;

    cb->size--;
    cb->total_consumed++;

    pthread_mutex_unlock(&cb->mutex);

    return CB_OK;
}

/* Retrieve buffer statistics */
int cb_get_stats(CircularBuffer *cb, BufferStats *stats)
{
    if (cb == NULL || stats == NULL || cb->buffer == NULL)
        return EINVAL;

    pthread_mutex_lock(&cb->mutex);

    stats->current_size = cb->size;
    stats->total_produced = cb->total_produced;
    stats->total_consumed = cb->total_consumed;
    stats->overflow_count = cb->overflow_count;

    pthread_mutex_unlock(&cb->mutex);

    return 0;
}

/* Close buffer and wake waiting threads */
int cb_close(CircularBuffer *cb)
{
    if (cb == NULL || cb->buffer == NULL)
        return EINVAL;

    pthread_mutex_lock(&cb->mutex);

    cb->closed = true;
    pthread_cond_broadcast(&cb->not_empty);

    pthread_mutex_unlock(&cb->mutex);

    return 0;
}

/* Release buffer resources */
void cb_destroy(CircularBuffer *cb)
{
    if (cb == NULL || cb->buffer == NULL)
        return;

    cb_close(cb);

    pthread_cond_destroy(&cb->not_empty);
    pthread_mutex_destroy(&cb->mutex);

    free(cb->buffer);
    memset(cb, 0, sizeof(*cb));
}

