/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#define _POSIX_C_SOURCE 200809L

#include "processor_thread.h"
#include "alert_thread.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define TEST_ALARM_QUEUE "/sms_alert_alarm_test"
#define TEST_LOG_QUEUE   "/sms_alert_log_test"

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", message); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static mqd_t create_queue(const char *name, long message_size)
{
    struct mq_attr attr;

    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = SMS_QUEUE_MAX_MESSAGES;
    attr.mq_msgsize = message_size;

    mq_unlink(name);

    mqd_t queue = mq_open(name,
                          O_CREAT | O_EXCL | O_RDWR,
                          0600,
                          &attr);

    CHECK(queue != (mqd_t)-1, "Create message queue");
    return queue;
}

int main(void)
{
    CircularBuffer cb;
    ProcessorThreadContext processor_ctx;
    AlertThreadContext alert_ctx;

    pthread_t processor_thread;
    pthread_t alert_thread;

    mqd_t alarm_queue;
    mqd_t log_queue;

    LogMessage log;
    mqd_t log_reader;
    int alert_log_count = 0;
    int data_log_count = 0;
    int stop_count = 0;

    CHECK(cb_init(&cb, SMS_BUFFER_CAPACITY) == 0,
          "Initialize circular buffer");

    alarm_queue = create_queue(TEST_ALARM_QUEUE,
                               sizeof(AlarmMessage));

    log_queue = create_queue(TEST_LOG_QUEUE,
                             sizeof(LogMessage));

    memset(&processor_ctx, 0, sizeof(processor_ctx));
    memset(&alert_ctx, 0, sizeof(alert_ctx));

    processor_ctx.cb = &cb;
    processor_ctx.alarm_queue = alarm_queue;
    processor_ctx.log_queue = log_queue;
    processor_ctx.temperature_threshold = 40.0f;
    processor_ctx.humidity_threshold = 80.0f;

    alert_ctx.alarm_queue = alarm_queue;
    alert_ctx.log_queue = log_queue;

    /* Start Alert before Processor to avoid blocking on a full queue. */
    CHECK(pthread_create(&alert_thread, NULL,
                         alert_thread_main, &alert_ctx) == 0,
          "Create alert thread");

    CHECK(pthread_create(&processor_thread, NULL,
                         processor_thread_main, &processor_ctx) == 0,
          "Create processor thread");

    SensorData samples[] = {
        {1000, 41.0f, 81.0f},
        {1500, 47.0f, 70.0f},
        {2000, 39.0f, 75.0f}
    };

    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        CHECK(cb_push(&cb, &samples[i]) == CB_OK,
              "Push sensor sample");
    }

    CHECK(cb_close(&cb) == 0, "Close circular buffer");

    CHECK(pthread_join(processor_thread, NULL) == 0,
          "Join processor thread");

    CHECK(pthread_join(alert_thread, NULL) == 0,
          "Join alert thread");

    CHECK(processor_ctx.error_code == 0,
          "Processor completed without error");

    CHECK(alert_ctx.error_code == 0,
          "Alert completed without error");

    log_reader = mq_open(TEST_LOG_QUEUE, O_RDONLY | O_NONBLOCK);
    CHECK(log_reader != (mqd_t)-1, "Open non-blocking log reader");

    while (mq_receive(log_reader, (char *)&log,
                      sizeof(log), NULL) >= 0) {
        if (log.type == SMS_LOG_DATA)
            data_log_count++;
        else if (log.type == SMS_LOG_ALERT)
            alert_log_count++;
        else if (log.type == SMS_LOG_STOP)
            stop_count++;
    }

    CHECK(errno == EAGAIN || errno == EWOULDBLOCK,
          "Log queue drained");

    CHECK(alert_ctx.temp_high_count == 2,
          "Temperature alert count");

    CHECK(alert_ctx.humidity_high_count == 1,
          "Humidity alert count");

    CHECK(alert_ctx.temp_change_count == 2,
          "Temperature change alert count");

    CHECK(data_log_count == 3, "Data log count");
    CHECK(alert_log_count == 5, "Alert log count");
    CHECK(stop_count == 1, "Logger stop message count");

    printf("Temperature alerts: %llu\n",
           (unsigned long long)alert_ctx.temp_high_count);
    printf("Humidity alerts: %llu\n",
           (unsigned long long)alert_ctx.humidity_high_count);
    printf("Temperature change alerts: %llu\n",
           (unsigned long long)alert_ctx.temp_change_count);
    printf("Data logs: %d\n", data_log_count);
    printf("Alert logs: %d\n", alert_log_count);
    printf("Stop messages: %d\n", stop_count);

    mq_close(log_reader);
    mq_close(alarm_queue);
    mq_close(log_queue);

    mq_unlink(TEST_ALARM_QUEUE);
    mq_unlink(TEST_LOG_QUEUE);

    cb_destroy(&cb);

    printf("ALL PROCESSOR + ALERT TESTS PASSED\n");
    return EXIT_SUCCESS;
}
