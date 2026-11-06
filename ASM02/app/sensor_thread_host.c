/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 * Host-only sensor simulation for application testing.
 */

#define _POSIX_C_SOURCE 200809L

#include "sensor_thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void sleep_interval(unsigned int interval_ms)
{
    struct timespec delay;

    delay.tv_sec = interval_ms / 1000;
    delay.tv_nsec = (long)(interval_ms % 1000) * 1000000L;

    while (nanosleep(&delay, &delay) < 0 && errno == EINTR) {
    }
}

static uint64_t get_timestamp_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL +
           (uint64_t)ts.tv_nsec / 1000000ULL;
}

void *sensor_thread_main(void *arg)
{
    SensorThreadContext *ctx = arg;
    unsigned int seed = (unsigned int)time(NULL);

    if (ctx == NULL || ctx->cb == NULL ||
        ctx->stop_requested == NULL ||
        ctx->interval_ms < 10) {
        if (ctx != NULL) {
            ctx->error_code = EINVAL;
            if (ctx->cb != NULL)
                cb_close(ctx->cb);
        }
        return NULL;
    }

    ctx->error_code = 0;

    printf("[SENSOR] Host simulation started\n");

    while (!atomic_load(ctx->stop_requested)) {
        SensorData sample;
        CBStatus status;

        sample.timestamp_ms = get_timestamp_ms();
        sample.temperature =
            15.0f + (float)(rand_r(&seed) % 3001) / 100.0f;
        sample.humidity =
            30.0f + (float)(rand_r(&seed) % 6001) / 100.0f;

        status = cb_push(ctx->cb, &sample);

        if (status == CB_FULL) {
            fprintf(stderr,
                    "[WARN] Circular buffer full; sample dropped\n");
        } else if (status == CB_CLOSED) {
            break;
        } else if (status != CB_OK) {
            ctx->error_code = EIO;
            break;
        } else if (ctx->verbose) {
            printf("[SENSOR] timestamp=%llu temp=%.2f humidity=%.2f\n",
                   (unsigned long long)sample.timestamp_ms,
                   sample.temperature,
                   sample.humidity);
        }

        sleep_interval(ctx->interval_ms);
    }

    cb_close(ctx->cb);
    printf("[SENSOR] Host simulation stopped\n");

    return NULL;
}
