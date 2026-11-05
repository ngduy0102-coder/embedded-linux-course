/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-05
 */

#define _POSIX_C_SOURCE 200809L

#include "sensor_thread.h"

#include "../driver/sms_sensor.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define SENSOR_LINE_SIZE 128

static void wait_retry_interval(unsigned int interval_ms)
{
    struct timespec delay;

    delay.tv_sec = interval_ms / 1000;
    delay.tv_nsec = (long)(interval_ms % 1000) * 1000000L;

    while (nanosleep(&delay, &delay) < 0 && errno == EINTR) {
        /* Resume remaining delay. */
    }
}

static int parse_sensor_data(const char *line, SensorData *data)
{
    unsigned long long timestamp;
    float temperature;
    float humidity;
    char extra;

    if (sscanf(line, "%llu,%f,%f %c",
               &timestamp, &temperature, &humidity, &extra) != 3)
        return -1;

    if (temperature < 15.0f || temperature > 45.0f ||
        humidity < 30.0f || humidity > 90.0f)
        return -1;

    data->timestamp_ms = (uint64_t)timestamp;
    data->temperature = temperature;
    data->humidity = humidity;

    return 0;
}

void *sensor_thread_main(void *arg)
{
    SensorThreadContext *ctx = arg;
    int fd;
    sms_u32 interval;
    char line[SENSOR_LINE_SIZE];

    if (ctx == NULL || ctx->cb == NULL ||
        ctx->stop_requested == NULL ||
        ctx->interval_ms < SMS_MIN_INTERVAL_MS) {
        if (ctx != NULL)
            ctx->error_code = EINVAL;
        return NULL;
    }

    ctx->error_code = 0;

    fd = open(SMS_SENSOR_DEVICE, O_RDONLY);
    if (fd < 0) {
        ctx->error_code = errno;
        perror("Sensor: open device");
        return NULL;
    }

    interval = ctx->interval_ms;

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        ctx->error_code = errno;
        perror("Sensor: set interval");
        close(fd);
        return NULL;
    }

    while (!atomic_load(ctx->stop_requested)) {
        ssize_t n;
        SensorData sample;
        CBStatus status;

        n = read(fd, line, sizeof(line) - 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN) {
                wait_retry_interval(ctx->interval_ms);
                continue;
            }

            ctx->error_code = errno;
            perror("Sensor: read");
            break;
        }

        if (n == 0) {
            ctx->error_code = EIO;
            fprintf(stderr, "Sensor: unexpected EOF\n");
            break;
        }

        line[n] = '\0';

        if (parse_sensor_data(line, &sample) < 0) {
            fprintf(stderr, "Sensor: invalid CSV: %s", line);
            continue;
        }

        status = cb_push(ctx->cb, &sample);

        if (status == CB_FULL) {
            fprintf(stderr,
                    "[WARN] Circular buffer full; sample dropped\n");
            continue;
        }

        if (status == CB_CLOSED) {
            break;
        }

        if (status != CB_OK) {
            ctx->error_code = EIO;
            fprintf(stderr, "Sensor: circular buffer error\n");
            break;
        }

        if (ctx->verbose) {
            printf("[SENSOR] timestamp=%llu temp=%.2f humidity=%.2f\n",
                   (unsigned long long)sample.timestamp_ms,
                   sample.temperature,
                   sample.humidity);
        }
    }

    if (close(fd) < 0 && ctx->error_code == 0)
        ctx->error_code = errno;

    return NULL;
}
