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
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define SENSOR_LINE_SIZE 128

#define SENSOR_FIFO_PRIORITY 50

static void add_interval(struct timespec *deadline,
                         unsigned int interval_ms)
{
    deadline->tv_sec += interval_ms / 1000;
    deadline->tv_nsec += (long)(interval_ms % 1000) * 1000000L;

    if (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec++;
        deadline->tv_nsec -= 1000000000L;
    }
}

static int sleep_until_deadline(const struct timespec *deadline,
                                atomic_bool *stop_requested)
{
    int rc;

    while (!atomic_load(stop_requested)) {
        rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                             deadline, NULL);

        if (rc == 0)
            return 0;

        if (rc != EINTR)
            return rc;
    }

    return 0;
}

static void configure_realtime_scheduling(void)
{
    struct sched_param param = {0};
    int rc;

    param.sched_priority = SENSOR_FIFO_PRIORITY;

    rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    if (rc == 0) {
        printf("[SENSOR] Scheduling: SCHED_FIFO, priority %d\n",
               SENSOR_FIFO_PRIORITY);
    } else {
        fprintf(stderr,
                "[WARN] SCHED_FIFO priority %d unavailable: %s\n",
                SENSOR_FIFO_PRIORITY, strerror(rc));
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
    struct timespec next_deadline = {0};
    bool deadline_valid = false;

    if (ctx == NULL || ctx->cb == NULL ||
        ctx->stop_requested == NULL ||
        ctx->interval_ms < SMS_MIN_INTERVAL_MS) {
        if (ctx != NULL) {
            ctx->error_code = EINVAL;
            if (ctx->cb != NULL)
                cb_close(ctx->cb);
        }
        return NULL;
    }

    ctx->error_code = 0;

    configure_realtime_scheduling();

    fd = open(SMS_SENSOR_DEVICE, O_RDONLY);
    if (fd < 0) {
        ctx->error_code = errno;
        perror("Sensor: open device");
        cb_close(ctx->cb);
        return NULL;
    }

    interval = ctx->interval_ms;

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        ctx->error_code = errno;
        perror("Sensor: set interval");
        close(fd);
        cb_close(ctx->cb);
        return NULL;
    }

    while (!atomic_load(ctx->stop_requested)) {
        ssize_t n;
        SensorData sample;
        CBStatus status;
        int sleep_rc;

        if (deadline_valid) {
            sleep_rc = sleep_until_deadline(&next_deadline,
                                            ctx->stop_requested);
            if (sleep_rc != 0) {
                ctx->error_code = sleep_rc;
                fprintf(stderr, "Sensor: clock_nanosleep: %s\n",
                        strerror(sleep_rc));
                break;
            }

            if (atomic_load(ctx->stop_requested))
                break;
        }

        n = read(fd, line, sizeof(line) - 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN) {
                /*
                 * The driver may reject a read slightly before its
                 * interval expires. Retry shortly without adding a
                 * full sampling period.
                 */
                if (clock_gettime(CLOCK_MONOTONIC,
                                  &next_deadline) < 0) {
                    ctx->error_code = errno;
                    perror("Sensor: clock_gettime");
                    break;
                }

                next_deadline.tv_nsec += 1000000L; /* 1 ms */
                if (next_deadline.tv_nsec >= 1000000000L) {
                    next_deadline.tv_sec++;
                    next_deadline.tv_nsec -= 1000000000L;
                }

                deadline_valid = true;
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
        } else if (status == CB_CLOSED) {
            break;
        } else if (status != CB_OK) {
            ctx->error_code = EIO;
            fprintf(stderr, "Sensor: circular buffer error\n");
            break;
        } else if (ctx->verbose) {
            printf("[SENSOR] timestamp=%llu temp=%.2f humidity=%.2f\n",
                   (unsigned long long)sample.timestamp_ms,
                   sample.temperature,
                   sample.humidity);
        }

        if (!deadline_valid) {
            if (clock_gettime(CLOCK_MONOTONIC, &next_deadline) < 0) {
                ctx->error_code = errno;
                perror("Sensor: clock_gettime");
                break;
            }
            deadline_valid = true;
        }

        add_interval(&next_deadline, ctx->interval_ms);
    }

    if (close(fd) < 0 && ctx->error_code == 0)
        ctx->error_code = errno;

    cb_close(ctx->cb);

    return NULL;
}
