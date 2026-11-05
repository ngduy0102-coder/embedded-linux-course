#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <stdbool.h>
#include <stdatomic.h>

#include "circular_buffer.h"

typedef struct {
    CircularBuffer *cb;

    unsigned int interval_ms;

    atomic_bool *stop_requested;

    bool verbose;

    int error_code;
} SensorThreadContext;

/* Sensor thread entry point */
void *sensor_thread_main(void *arg);

#endif /* SENSOR_THREAD_H */
