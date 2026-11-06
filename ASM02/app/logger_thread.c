#define _POSIX_C_SOURCE 200809L

/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#include "logger_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#define SMS_LOG_MAX_SIZE (1024 * 1024)
#define SMS_LOG_FLUSH_INTERVAL 5
#define SMS_LOG_PATH_MAX 512

static const char *get_alarm_name(AlarmType type)
{
    switch (type) {
    case SMS_ALARM_TEMP_HIGH:
        return "TEMP_HIGH";
    case SMS_ALARM_HUMIDITY_HIGH:
        return "HUMIDITY_HIGH";
    case SMS_ALARM_TEMP_CHANGE:
        return "TEMP_CHANGE";
    default:
        return "UNKNOWN";
    }
}

static int write_data_log(FILE *file, const LogMessage *message)
{
    if (fprintf(file,
                "[%llu] DATA: TEMP=%.2f C, HUMIDITY=%.2f%%, "
                "AVG_TEMP=%.2f C, AVG_HUMIDITY=%.2f%%\n",
                (unsigned long long)message->timestamp_ms,
                message->temperature,
                message->humidity,
                message->avg_temperature,
                message->avg_humidity) < 0) {
        return -1;
    }

    return 0;
}

static int write_alert_log(FILE *file, const LogMessage *message)
{
    if (fprintf(file,
                "[ALERT][%llu] %s: VALUE: %.2f THRESHOLD: %.2f\n",
                (unsigned long long)message->timestamp_ms,
                get_alarm_name(message->alarm_type),
                message->value,
                message->threshold) < 0) {
        return -1;
    }

    return 0;
}

static int rotate_log_file(FILE **file, const char *log_path)
{
    char backup_path[SMS_LOG_PATH_MAX];
    int length;

    length = snprintf(backup_path, sizeof(backup_path),
                      "%s.1", log_path);

    if (length < 0 || (size_t)length >= sizeof(backup_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    if (fflush(*file) != 0)
        return -1;

    if (fclose(*file) != 0) {
        *file = NULL;
        return -1;
    }

    *file = NULL;

    if (remove(backup_path) != 0 && errno != ENOENT)
        return -1;

    if (rename(log_path, backup_path) != 0)
        return -1;

    *file = fopen(log_path, "w");

    if (*file == NULL)
        return -1;

    return 0;
}

static int check_log_rotation(FILE **file, const char *log_path)
{
    off_t current_size;

    if (*file == NULL) {
        errno = EBADF;
        return -1;
    }

    current_size = ftello(*file);

    if (current_size < 0)
        return -1;

    if (current_size > SMS_LOG_MAX_SIZE) {
        if (rotate_log_file(file, log_path) != 0)
            return -1;
    }

    return 0;
}

static int write_log_message(FILE **file,
                             const char *log_path,
                             const LogMessage *message)
{
    int result;

    switch (message->type) {
    case SMS_LOG_DATA:
        result = write_data_log(*file, message);
        break;

    case SMS_LOG_ALERT:
        result = write_alert_log(*file, message);
        break;

    default:
        errno = EINVAL;
        return -1;
    }

    if (result != 0)
        return -1;

    /* Check rotation after writing the message. */
    return check_log_rotation(file, log_path);
}

void *logger_thread_main(void *arg)
{
    LoggerThreadContext *ctx = (LoggerThreadContext *)arg;
    FILE *file = NULL;
    LogMessage message;
    struct timespec next_flush;
    int received;
    int stop_requested = 0;

    if (ctx == NULL)
        return NULL;

    ctx->error_code = 0;

    if (ctx->log_path == NULL) {
        ctx->error_code = EINVAL;
        return NULL;
    }

    file = fopen(ctx->log_path, "a");

    if (file == NULL) {
        ctx->error_code = errno;
        perror("Logger: fopen");
        return NULL;
    }

    fprintf(stdout, "[LOGGER] Started: %s\n", ctx->log_path);

    if (clock_gettime(CLOCK_REALTIME, &next_flush) != 0) {
        ctx->error_code = errno;
        goto cleanup;
    }

    next_flush.tv_sec += SMS_LOG_FLUSH_INTERVAL;

    while (!stop_requested) {
        received = mq_timedreceive(
            ctx->log_queue,
            (char *)&message,
            sizeof(message),
            NULL,
            &next_flush
        );

        if (received < 0) {
            if (errno == EINTR)
                continue;

            if (errno == ETIMEDOUT) {
                if (fflush(file) != 0) {
                    ctx->error_code = errno;
                    break;
                }

                if (clock_gettime(CLOCK_REALTIME, &next_flush) != 0) {
                    ctx->error_code = errno;
                    break;
                }

                next_flush.tv_sec += SMS_LOG_FLUSH_INTERVAL;
                continue;
            }

            ctx->error_code = errno;
            perror("Logger: mq_timedreceive");
            break;
        }

        if ((size_t)received != sizeof(message)) {
            ctx->error_code = EBADMSG;
            break;
        }

        if (message.type == SMS_LOG_STOP) {
            stop_requested = 1;
            continue;
        }

        if (write_log_message(&file, ctx->log_path, &message) != 0) {
            ctx->error_code = errno ? errno : EIO;
            perror("Logger: write_log_message");
            break;
        }

        /*
         * Flush when the deadline has passed, even if messages
         * keep arriving continuously.
         */
        {
            struct timespec now;

            if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
                ctx->error_code = errno;
                break;
            }

            if (now.tv_sec > next_flush.tv_sec ||
                (now.tv_sec == next_flush.tv_sec &&
                 now.tv_nsec >= next_flush.tv_nsec)) {

                if (fflush(file) != 0) {
                    ctx->error_code = errno;
                    break;
                }

                if (clock_gettime(CLOCK_REALTIME, &next_flush) != 0) {
                    ctx->error_code = errno;
                    break;
                }

                next_flush.tv_sec += SMS_LOG_FLUSH_INTERVAL;
            }
        }
    }

cleanup:
    if (file != NULL) {
        if (fflush(file) != 0 && ctx->error_code == 0)
            ctx->error_code = errno;

        if (fclose(file) != 0 && ctx->error_code == 0)
            ctx->error_code = errno;
    }

    fprintf(stdout, "[LOGGER] Stopped\n");

    return NULL;
}
