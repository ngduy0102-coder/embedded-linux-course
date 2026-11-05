#define _POSIX_C_SOURCE 200809L

#include "circular_buffer.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    CircularBuffer *cb;
    atomic_bool started;
    CBStatus result;
} ConsumerArgs;

static void *consumer_thread(void *arg)
{
    ConsumerArgs *args = arg;
    SensorData data;

    atomic_store(&args->started, true);
    args->result = cb_pop(args->cb, &data);

    return NULL;
}

#define CHECK(condition, message)                 \
    do {                                           \
        if (!(condition)) {                        \
            fprintf(stderr, "FAIL: %s\n", message);\
            return 1;                              \
        }                                          \
        printf("PASS: %s\n", message);             \
    } while (0)

int main(void)
{
    CircularBuffer cb;
    SensorData input1 = {1000, 25.5f, 60.0f};
    SensorData input2 = {1500, 26.0f, 61.0f};
    SensorData input3 = {2000, 27.0f, 62.0f};
    SensorData output;
    BufferStats stats;

    CHECK(cb_init(&cb, 2) == 0, "Initialize buffer");

    CHECK(cb_push(&cb, &input1) == CB_OK, "Push first sample");
    CHECK(cb_push(&cb, &input2) == CB_OK, "Push second sample");
    CHECK(cb_push(&cb, &input3) == CB_FULL, "Detect full buffer");

    CHECK(cb_get_stats(&cb, &stats) == 0, "Read buffer statistics");
    CHECK(stats.current_size == 2, "Buffer size is 2");
    CHECK(stats.total_produced == 2, "Produced count is 2");
    CHECK(stats.overflow_count == 1, "Overflow count is 1");

    CHECK(cb_pop(&cb, &output) == CB_OK, "Pop first sample");
    CHECK(output.timestamp_ms == 1000, "FIFO order: first sample");

    CHECK(cb_pop(&cb, &output) == CB_OK, "Pop second sample");
    CHECK(output.timestamp_ms == 1500, "FIFO order: second sample");

    ConsumerArgs args = {
        .cb = &cb,
        .started = ATOMIC_VAR_INIT(false),
        .result = CB_ERROR
    };

    pthread_t thread;

    CHECK(pthread_create(&thread, NULL, consumer_thread, &args) == 0,
          "Create waiting consumer");

    while (!atomic_load(&args.started)) {
        struct timespec delay = {0, 1000000L};
        nanosleep(&delay, NULL);
    }

    {
        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    CHECK(cb_close(&cb) == 0, "Close buffer and wake consumer");

    CHECK(pthread_join(thread, NULL) == 0, "Join consumer thread");

    CHECK(args.result == CB_CLOSED, "Consumer receives CB_CLOSED");

    CHECK(cb_push(&cb, &input3) == CB_CLOSED,
          "Reject push after close");

    CHECK(cb_get_stats(&cb, &stats) == 0, "Read final statistics");
    CHECK(stats.total_consumed == 2, "Consumed count is 2");

    cb_destroy(&cb);

    printf("\nALL CIRCULAR BUFFER TESTS PASSED\n");
    return 0;
}

