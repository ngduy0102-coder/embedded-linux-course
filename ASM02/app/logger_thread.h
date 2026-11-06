#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include "sms_common.h"
#include <mqueue.h>

typedef struct {
    mqd_t log_queue;
    const char *log_path;
    int error_code;
} LoggerThreadContext;

void *logger_thread_main(void *arg);

#endif
