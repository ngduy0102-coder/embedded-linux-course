/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#define _POSIX_C_SOURCE 200809L

#include "processor_thread.h"

#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h>

#define TEST_ALARM_QUEUE "/sms_processor_alarm_test"
#define TEST_LOG_QUEUE   "/sms_processor_log_test"

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
                          O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK,
                          0600,
                          &attr);

    CHECK(queue != (mqd_t)-1, "Create message queue");
    return queue;
}

int main(void)
{
    CircularBuffer cb;
    ProcessorThreadContext ctx;
    pthread_t processor;

    AlarmMessage alarm;
    LogMessage log;

    int alarm_count = 0;
    int log_count = 0;
    int stop_count = 0;
    int temp_high_count = 0;
    int humidity_high_count = 0;
    int temp_change_count = 0;

    mqd_t alarm_queue;
    mqd_t log_queue;

    CHECK(cb_init(&cb, SMS_BUFFER_CAPACITY) == 0,
          "Initialize circular buffer");

    alarm_queue = create_queue(TEST_ALARM_QUEUE,
                               sizeof(AlarmMessage));

    log_queue = create_queue(TEST_LOG_QUEUE,
                             sizeof(LogMessage));

    memset(&ctx, 0, sizeof(ctx));

    ctx.cb = &cb;
    ctx.alarm_queue = alarm_queue;
    ctx.log_queue = log_queue;
    ctx.temperature_threshold = 40.0f;
    ctx.humidity_threshold = 80.0f;

    CHECK(pthread_create(&processor, NULL,
                         processor_thread_main, &ctx) == 0,
          "Create processor thread");

    SensorData samples[] = {
        {1000, 41.0f, 81.0f},
        {1500, 47.0f, 70.0f},
        {2000, 39.0f, 75.0f}
    };

    for (size_t i = 0;
         i < sizeof(samples) / sizeof(samples[0]);
         i++) {
        CHECK(cb_push(&cb, &samples[i]) == CB_OK,
              "Push sensor sample");
    }

    CHECK(cb_close(&cb) == 0, "Close circular buffer");

    CHECK(pthread_join(processor, NULL) == 0,
          "Join processor thread");

    CHECK(ctx.error_code == 0, "Processor completed without error");

    while (mq_receive(alarm_queue, (char *)&alarm,
                      sizeof(alarm), NULL) >= 0) {
        alarm_count++;

        switch (alarm.type) {
        case SMS_ALARM_TEMP_HIGH:
            temp_high_count++;
            break;

        case SMS_ALARM_HUMIDITY_HIGH:
            humidity_high_count++;
            break;

        case SMS_ALARM_TEMP_CHANGE:
            temp_change_count++;
            break;

        case SMS_ALARM_STOP:
            stop_count++;
            break;

        default:
            CHECK(0, "Unknown alarm type");
        }
    }

    CHECK(errno == EAGAIN || errno == EWOULDBLOCK,
          "Alarm queue drained");

    while (mq_receive(log_queue, (char *)&log,
                      sizeof(log), NULL) >= 0) {
        log_count++;

        if (log.type == SMS_LOG_DATA && log.timestamp_ms == 2000) {
            CHECK(fabsf(log.avg_temperature - 42.3333f) < 0.01f,
                  "Third sample rolling temperature average");

            CHECK(fabsf(log.avg_humidity - 75.3333f) < 0.01f,
                  "Third sample rolling humidity average");
        }
    }

    CHECK(errno == EAGAIN || errno == EWOULDBLOCK,
          "Log queue drained");

    CHECK(alarm_count == 6, "Expected 5 alarms and 1 stop message");
    CHECK(temp_high_count == 2, "Temperature high alarm count");
    CHECK(humidity_high_count == 1, "Humidity high alarm count");
    CHECK(temp_change_count == 2, "Temperature change alarm count");
    CHECK(stop_count == 1, "Processor stop message count");
    CHECK(log_count == 3, "Processed log count");

    printf("Alarm messages: %d\n", alarm_count);
    printf("Temperature high: %d\n", temp_high_count);
    printf("Humidity high: %d\n", humidity_high_count);
    printf("Temperature change: %d\n", temp_change_count);
    printf("Stop messages: %d\n", stop_count);
    printf("Processed logs: %d\n", log_count);

    mq_close(alarm_queue);
    mq_close(log_queue);

    mq_unlink(TEST_ALARM_QUEUE);
    mq_unlink(TEST_LOG_QUEUE);

    cb_destroy(&cb);

    printf("ALL PROCESSOR THREAD TESTS PASSED\n");
    return EXIT_SUCCESS;
}
