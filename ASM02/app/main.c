/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"
#include "../driver/sms_sensor.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <signal.h>
#include <getopt.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <sys/mman.h>

#define DEFAULT_INTERVAL_MS 500
#define DEFAULT_TEMP_THRESHOLD 40.0f
#define DEFAULT_HUMIDITY_THRESHOLD 80.0f
#define DEFAULT_DURATION_SEC 60
#define LOG_FILE_PATH "/tmp/sms.log"

static volatile sig_atomic_t signal_received = 0;

static void handle_signal(int signo)
{
    (void)signo;
    signal_received = 1;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "  -i <ms>    Sampling interval (default: 500ms)\n"
        "  -t <°C>    Temperature threshold (default: 40.0)\n"
        "  -h <%%>     Humidity threshold (default: 80.0)\n"
        "  -d <sec>   Run duration, 0 = infinite (default: 60)\n"
        "  -v         Verbose sensor output\n",
        program);
}

static int parse_options(int argc, char **argv,
                         unsigned int *interval,
                         float *temp_threshold,
                         float *humidity_threshold,
                         unsigned int *duration,
                         bool *verbose)
{
    int opt;
    char *end;

    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (opt) {
        case 'i': {
            unsigned long value;

            errno = 0;
            value = strtoul(optarg, &end, 10);

            if (errno || *end != '\0' ||
                value < SMS_MIN_INTERVAL_MS || value > UINT_MAX)
                return -1;

            *interval = (unsigned int)value;
            break;
        }

        case 't':
            errno = 0;
            *temp_threshold = strtof(optarg, &end);

            if (errno || *end != '\0' ||
                !isfinite(*temp_threshold) || *temp_threshold <= 0)
                return -1;
            break;

        case 'h':
            errno = 0;
            *humidity_threshold = strtof(optarg, &end);

            if (errno || *end != '\0' ||
                !isfinite(*humidity_threshold) ||
                *humidity_threshold < 0 ||
                *humidity_threshold > 100)
                return -1;
            break;

        case 'd': {
            unsigned long value;

            errno = 0;
            value = strtoul(optarg, &end, 10);

            if (errno || *end != '\0' || value > UINT_MAX)
                return -1;

            *duration = (unsigned int)value;
            break;
        }

        case 'v':
            *verbose = true;
            break;

        default:
            return -1;
        }
    }

    return optind == argc ? 0 : -1;
}

static int send_alarm_stop(mqd_t queue)
{
    AlarmMessage message = {0};

    message.type = SMS_ALARM_STOP;

    while (mq_send(queue, (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno != EINTR)
            return -1;
    }

    return 0;
}

static int send_log_stop(mqd_t queue)
{
    LogMessage message = {0};

    message.type = SMS_LOG_STOP;

    while (mq_send(queue, (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno != EINTR)
            return -1;
    }

    return 0;
}

static void wait_for_shutdown(unsigned int duration)
{
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 100000000L
    };
    unsigned int elapsed = 0;

    while (!signal_received &&
           (duration == 0 || elapsed < duration * 10U)) {
        nanosleep(&delay, NULL);
        elapsed++;
    }
}

int main(int argc, char **argv)
{
    unsigned int interval = DEFAULT_INTERVAL_MS;
    float temp_threshold = DEFAULT_TEMP_THRESHOLD;
    float humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
    unsigned int duration = DEFAULT_DURATION_SEC;
    bool verbose = false;

    struct sigaction action = {0};
    struct mq_attr alarm_attr = {0};
    struct mq_attr log_attr = {0};

    mqd_t alarm_queue = (mqd_t)-1;
    mqd_t log_queue = (mqd_t)-1;

    CircularBuffer buffer;
    bool buffer_initialized = false;

    pthread_t logger_tid, alert_tid, processor_tid, sensor_tid;
    bool logger_started = false;
    bool alert_started = false;
    bool processor_started = false;
    bool sensor_started = false;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    LoggerThreadContext logger_ctx = {0};
    AlertThreadContext alert_ctx = {0};
    ProcessorThreadContext processor_ctx = {0};
    SensorThreadContext sensor_ctx = {0};

    int exit_status = EXIT_SUCCESS;

    if (parse_options(argc, argv, &interval,
                      &temp_threshold, &humidity_threshold,
                      &duration, &verbose) != 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    alarm_attr.mq_maxmsg = SMS_QUEUE_MAX_MESSAGES;
    alarm_attr.mq_msgsize = sizeof(AlarmMessage);

    log_attr.mq_maxmsg = SMS_QUEUE_MAX_MESSAGES;
    log_attr.mq_msgsize = sizeof(LogMessage);

    alarm_queue = mq_open(SMS_ALARM_QUEUE,
                          O_CREAT | O_EXCL | O_RDWR,
                          0600, &alarm_attr);
    if (alarm_queue == (mqd_t)-1) {
        perror("mq_open alarm_queue");
        return EXIT_FAILURE;
    }

    log_queue = mq_open(SMS_LOG_QUEUE,
                        O_CREAT | O_EXCL | O_RDWR,
                        0600, &log_attr);
    if (log_queue == (mqd_t)-1) {
        perror("mq_open log_queue");
        mq_close(alarm_queue);
        mq_unlink(SMS_ALARM_QUEUE);
        return EXIT_FAILURE;
    }

    if (cb_init(&buffer, SMS_BUFFER_CAPACITY) != 0) {
        fprintf(stderr, "Failed to initialize circular buffer\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    buffer_initialized = true;

    logger_ctx.log_queue = log_queue;
    logger_ctx.log_path = LOG_FILE_PATH;

    alert_ctx.alarm_queue = alarm_queue;
    alert_ctx.log_queue = log_queue;

    processor_ctx.cb = &buffer;
    processor_ctx.alarm_queue = alarm_queue;
    processor_ctx.log_queue = log_queue;
    processor_ctx.temperature_threshold = temp_threshold;
    processor_ctx.humidity_threshold = humidity_threshold;

    sensor_ctx.cb = &buffer;
    sensor_ctx.interval_ms = interval;
    sensor_ctx.stop_requested = &stop_requested;
    sensor_ctx.verbose = verbose;

    /* Lock current and future memory pages for real-time operation. */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
        printf("[MAIN] mlockall: enabled\n");
    } else {
        perror("[MAIN] mlockall");
        fprintf(stderr, "[WARN] Continuing without locked memory\n");
    }

    /* Start Logger first. */
    if (pthread_create(&logger_tid, NULL,
                       logger_thread_main, &logger_ctx) != 0) {
        fprintf(stderr, "Failed to create Logger thread\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    logger_started = true;

    if (pthread_create(&alert_tid, NULL,
                       alert_thread_main, &alert_ctx) != 0) {
        fprintf(stderr, "Failed to create Alert thread\n");
        exit_status = EXIT_FAILURE;
        send_log_stop(log_queue);
        goto join_threads;
    }
    alert_started = true;

    if (pthread_create(&processor_tid, NULL,
                       processor_thread_main, &processor_ctx) != 0) {
        fprintf(stderr, "Failed to create Processor thread\n");
        exit_status = EXIT_FAILURE;
        send_alarm_stop(alarm_queue);
        goto join_threads;
    }
    processor_started = true;

    if (pthread_create(&sensor_tid, NULL,
                       sensor_thread_main, &sensor_ctx) != 0) {
        fprintf(stderr, "Failed to create Sensor thread\n");
        exit_status = EXIT_FAILURE;
        cb_close(&buffer);
        goto join_threads;
    }
    sensor_started = true;

    printf("[MAIN] Sensor Monitoring System started\n");
    printf("[MAIN] Interval=%u ms, Temp=%.2f C, Humidity=%.2f%%, Duration=%u s\n",
           interval, temp_threshold, humidity_threshold, duration);

    wait_for_shutdown(duration);

    printf("\n[MAIN] Shutdown requested\n");
    atomic_store(&stop_requested, true);

join_threads:
    if (sensor_started)
        pthread_join(sensor_tid, NULL);

    if (processor_started)
        pthread_join(processor_tid, NULL);

    if (alert_started)
        pthread_join(alert_tid, NULL);

    if (logger_started)
        pthread_join(logger_tid, NULL);

    if (sensor_ctx.error_code ||
        processor_ctx.error_code ||
        alert_ctx.error_code ||
        logger_ctx.error_code) {
        fprintf(stderr,
                "[MAIN] Thread errors: sensor=%d processor=%d "
                "alert=%d logger=%d\n",
                sensor_ctx.error_code,
                processor_ctx.error_code,
                alert_ctx.error_code,
                logger_ctx.error_code);
        exit_status = EXIT_FAILURE;
    }

    if (buffer_initialized) {
        BufferStats stats;

        if (cb_get_stats(&buffer, &stats) == 0) {
            printf("[MAIN] Buffer: produced=%llu consumed=%llu "
                   "overflow=%llu remaining=%zu\n",
                   (unsigned long long)stats.total_produced,
                   (unsigned long long)stats.total_consumed,
                   (unsigned long long)stats.overflow_count,
                   stats.current_size);
        }
    }

cleanup:
    if (buffer_initialized)
        cb_destroy(&buffer);

    if (alarm_queue != (mqd_t)-1)
        mq_close(alarm_queue);

    if (log_queue != (mqd_t)-1)
        mq_close(log_queue);

    mq_unlink(SMS_ALARM_QUEUE);
    mq_unlink(SMS_LOG_QUEUE);

    printf("[MAIN] Application exited\n");
    return exit_status;
}
