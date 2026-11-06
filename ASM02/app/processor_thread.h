#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include "circular_buffer.h"
#include "sms_common.h"

#include <mqueue.h>

typedef struct {
    CircularBuffer *cb;

    mqd_t alarm_queue;
    mqd_t log_queue;

    float temperature_threshold;
    float humidity_threshold;

    int error_code;
} ProcessorThreadContext;

void *processor_thread_main(void *arg);

#endif /* PROCESSOR_THREAD_H */
