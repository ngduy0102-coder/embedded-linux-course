#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include "sms_common.h"

#include <mqueue.h>
#include <stdint.h>

typedef struct {
    mqd_t alarm_queue;
    mqd_t log_queue;

    uint64_t temp_high_count;
    uint64_t humidity_high_count;
    uint64_t temp_change_count;

    int error_code;
} AlertThreadContext;

void *alert_thread_main(void *arg);

#endif /* ALERT_THREAD_H */
