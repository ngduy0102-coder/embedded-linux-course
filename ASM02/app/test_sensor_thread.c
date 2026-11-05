/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-05
 */

#define _POSIX_C_SOURCE 200809L

#include "sensor_thread.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#define TEST_DURATION_SEC 3

int main(void)
{
    CircularBuffer cb;
    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);
    SensorThreadContext context = {
        .cb = &cb,
        .interval_ms = 500,
        .stop_requested = &stop_requested,
        .verbose = true,
        .error_code = 0
    };

    pthread_t sensor_thread;
    BufferStats stats;
    struct timespec duration = {
        .tv_sec = TEST_DURATION_SEC,
        .tv_nsec = 0
    };

    printf("===== SENSOR THREAD TEST =====\n");

    /* Initialize circular buffer */
    if (cb_init(&cb, SMS_BUFFER_CAPACITY) != 0) {
        fprintf(stderr, "FAIL: Circular buffer initialization\n");
        return 1;
    }

    printf("[PASS] Circular buffer initialized\n");

    /* Start sensor thread */
    int ret = pthread_create(
        &sensor_thread,
        NULL,
        sensor_thread_main,
        &context
    );

    if (ret != 0) {
        fprintf(stderr, "FAIL: pthread_create: %d\n", ret);
        cb_destroy(&cb);
        return 1;
    }

    printf("[PASS] Sensor thread started\n");

    /* Run sensor thread for 3 seconds */
    while (nanosleep(&duration, &duration) < 0) {
        if (errno != EINTR) {
            perror("nanosleep");
            break;
        }
    }

    /* Request graceful shutdown */
    atomic_store(&stop_requested, true);

    ret = pthread_join(sensor_thread, NULL);

    if (ret != 0) {
        fprintf(stderr, "FAIL: pthread_join: %d\n", ret);
        cb_close(&cb);
        cb_destroy(&cb);
        return 1;
    }

    printf("[PASS] Sensor thread stopped\n");

    /* Check sensor thread error */
    if (context.error_code != 0) {
        fprintf(stderr, "FAIL: Sensor thread error: %d\n",
                context.error_code);
    }

    /* Get circular buffer statistics */
    if (cb_get_stats(&cb, &stats) != 0) {
        fprintf(stderr, "FAIL: Cannot get buffer statistics\n");
        cb_destroy(&cb);
        return 1;
    }

    printf("\n===== SENSOR STATISTICS =====\n");
    printf("Total produced : %llu\n",
           (unsigned long long)stats.total_produced);
    printf("Total consumed : %llu\n",
           (unsigned long long)stats.total_consumed);
    printf("Current size   : %zu\n", stats.current_size);
    printf("Overflow count : %llu\n",
           (unsigned long long)stats.overflow_count);

    /* Validate results */
    if (stats.total_produced == 0) {
        fprintf(stderr, "FAIL: No sensor samples received\n");
    } else if (context.error_code != 0) {
        fprintf(stderr, "FAIL: Sensor thread did not finish cleanly\n");
    } else {
        printf("\nALL SENSOR THREAD TESTS PASSED\n");
    }

    cb_destroy(&cb);

    return (stats.total_produced > 0 && context.error_code == 0) ? 0 : 1;
}
