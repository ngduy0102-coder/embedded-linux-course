/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#define _POSIX_C_SOURCE 200809L

#include "processor_thread.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ROLLING_WINDOW_SIZE 10
#define TEMP_CHANGE_THRESHOLD 5.0f

static int send_alarm(ProcessorThreadContext *ctx,
                      AlarmType type,
                      const SensorData *sample,
                      float value,
                      float threshold)
{
    AlarmMessage message;

    memset(&message, 0, sizeof(message));

    message.type = type;
    message.timestamp_ms = sample->timestamp_ms;
    message.value = value;
    message.threshold = threshold;

    while (mq_send(ctx->alarm_queue,
                   (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno == EINTR)
            continue;

        return errno;
    }

    return 0;
}

static int send_log(ProcessorThreadContext *ctx,
                    const SensorData *sample,
                    float avg_temperature,
                    float avg_humidity)
{
    LogMessage message;

    memset(&message, 0, sizeof(message));

    message.type = SMS_LOG_DATA;
    message.timestamp_ms = sample->timestamp_ms;
    message.temperature = sample->temperature;
    message.humidity = sample->humidity;
    message.avg_temperature = avg_temperature;
    message.avg_humidity = avg_humidity;

    while (mq_send(ctx->log_queue,
                   (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno == EINTR)
            continue;

        return errno;
    }

    return 0;
}

static int send_stop_message(ProcessorThreadContext *ctx)
{
    AlarmMessage message;

    memset(&message, 0, sizeof(message));
    message.type = SMS_ALARM_STOP;

    while (mq_send(ctx->alarm_queue,
                   (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno == EINTR)
            continue;

        return errno;
    }

    return 0;
}

void *processor_thread_main(void *arg)
{
    ProcessorThreadContext *ctx = arg;

    float temperatures[ROLLING_WINDOW_SIZE] = {0};
    float humidities[ROLLING_WINDOW_SIZE] = {0};

    size_t window_count = 0;
    size_t window_index = 0;

    float previous_temperature = 0.0f;
    int has_previous_temperature = 0;

    if (ctx == NULL || ctx->cb == NULL) {
        return NULL;
    }

    ctx->error_code = 0;

    for (;;) {
        SensorData sample;
        CBStatus status;

        status = cb_pop(ctx->cb, &sample);

        if (status == CB_CLOSED)
            break;

        if (status != CB_OK) {
            ctx->error_code = EIO;
            fprintf(stderr, "Processor: circular buffer error\n");
            break;
        }

        temperatures[window_index] = sample.temperature;
        humidities[window_index] = sample.humidity;

        window_index = (window_index + 1) % ROLLING_WINDOW_SIZE;

        if (window_count < ROLLING_WINDOW_SIZE)
            window_count++;

        float temp_sum = 0.0f;
        float humidity_sum = 0.0f;

        for (size_t i = 0; i < window_count; i++) {
            temp_sum += temperatures[i];
            humidity_sum += humidities[i];
        }

        float avg_temperature = temp_sum / (float)window_count;
        float avg_humidity = humidity_sum / (float)window_count;

        if (sample.temperature > ctx->temperature_threshold) {
            int ret = send_alarm(ctx, SMS_ALARM_TEMP_HIGH,
                                 &sample,
                                 sample.temperature,
                                 ctx->temperature_threshold);
            if (ret != 0) {
                ctx->error_code = ret;
                break;
            }
        }

        if (sample.humidity > ctx->humidity_threshold) {
            int ret = send_alarm(ctx, SMS_ALARM_HUMIDITY_HIGH,
                                 &sample,
                                 sample.humidity,
                                 ctx->humidity_threshold);
            if (ret != 0) {
                ctx->error_code = ret;
                break;
            }
        }

        if (has_previous_temperature &&
            fabsf(sample.temperature - previous_temperature) >
                TEMP_CHANGE_THRESHOLD) {

            int ret = send_alarm(ctx, SMS_ALARM_TEMP_CHANGE,
                                 &sample,
                                 fabsf(sample.temperature -
                                       previous_temperature),
                                 TEMP_CHANGE_THRESHOLD);
            if (ret != 0) {
                ctx->error_code = ret;
                break;
            }
        }

        previous_temperature = sample.temperature;
        has_previous_temperature = 1;

        if (send_log(ctx, &sample,
                     avg_temperature, avg_humidity) != 0) {
            ctx->error_code = errno;
            break;
        }
    }

    if (send_stop_message(ctx) != 0 && ctx->error_code == 0)
        ctx->error_code = errno;

    return NULL;
}
