/*
 * Real-Time Jitter Benchmark
 * Student ID: SE192916
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>

#define SAMPLE_COUNT 1000
#define PERIOD_NS 1000000L

static int64_t jitter_ns[SAMPLE_COUNT];

static int compare_int64(const void *a, const void *b)
{
    int64_t x = *(const int64_t *)a;
    int64_t y = *(const int64_t *)b;

    return (x > y) - (x < y);
}

static void add_period(struct timespec *t)
{
    t->tv_nsec += PERIOD_NS;

    if (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

static int64_t diff_ns(const struct timespec *a,
                       const struct timespec *b)
{
    return ((int64_t)a->tv_sec - (int64_t)b->tv_sec)
           * 1000000000LL
           + (int64_t)a->tv_nsec - (int64_t)b->tv_nsec;
}

int main(int argc, char *argv[])
{
    int policy;
    struct sched_param param = {0};
    struct timespec deadline, actual;
    int64_t min, max, sum = 0;
    unsigned int histogram[4] = {0};
    const char *mode;

    if (argc != 2 ||
        (strcmp(argv[1], "other") != 0 &&
         strcmp(argv[1], "fifo") != 0)) {
        fprintf(stderr, "Usage: %s <other|fifo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "fifo") == 0) {
        policy = SCHED_FIFO;
        param.sched_priority = 50;
        mode = "SCHED_FIFO";
    } else {
        policy = SCHED_OTHER;
        param.sched_priority = 0;
        mode = "SCHED_OTHER";
    }

    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0)
        printf("mlockall: SUCCESS\n");
    else
        perror("mlockall");

    if (sched_setscheduler(0, policy, &param) < 0) {
        perror("sched_setscheduler");
        return EXIT_FAILURE;
    }

    printf("\nMode: %s\n", mode);
    printf("Priority: %d\n", param.sched_priority);
    printf("Samples: %d\n", SAMPLE_COUNT);
    printf("Period: 1 ms\n\n");

    if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int rc;

        add_period(&deadline);

        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC,
                                 TIMER_ABSTIME,
                                 &deadline, NULL);
        } while (rc == EINTR);

        if (rc != 0) {
            fprintf(stderr, "clock_nanosleep: %s\n", strerror(rc));
            return EXIT_FAILURE;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &actual) < 0) {
            perror("clock_gettime");
            return EXIT_FAILURE;
        }

        jitter_ns[i] = diff_ns(&actual, &deadline);

        if (jitter_ns[i] < 0)
            jitter_ns[i] = 0;
    }

    qsort(jitter_ns, SAMPLE_COUNT,
          sizeof(jitter_ns[0]), compare_int64);

    min = jitter_ns[0];
    max = jitter_ns[SAMPLE_COUNT - 1];

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += jitter_ns[i];

        if (jitter_ns[i] < 100000)
            histogram[0]++;
        else if (jitter_ns[i] < 500000)
            histogram[1]++;
        else if (jitter_ns[i] < 1000000)
            histogram[2]++;
        else
            histogram[3]++;
    }

    printf("===== JITTER RESULTS =====\n");
    printf("Min:     %.2f us\n", min / 1000.0);
    printf("Average: %.2f us\n",
           (double)sum / SAMPLE_COUNT / 1000.0);
    printf("Max:     %.2f us\n", max / 1000.0);
    printf("P99:     %.2f us\n",
           jitter_ns[989] / 1000.0);

    printf("\n===== HISTOGRAM =====\n");
    printf("0-100 us:       %u\n", histogram[0]);
    printf("100-500 us:     %u\n", histogram[1]);
    printf("500-1000 us:    %u\n", histogram[2]);
    printf(">1000 us:       %u\n", histogram[3]);

    return EXIT_SUCCESS;
}
