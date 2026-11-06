/*
 * Author: Nguyen Duy
 * Student ID: SE192916
 * Date: 2026-11-06
 */

#define _POSIX_C_SOURCE 200809L

#include "alert_thread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

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

static void count_alarm(AlertThreadContext *ctx, AlarmType type)
{
    switch (type) {
    case SMS_ALARM_TEMP_HIGH:
        ctx->temp_high_count++;
        break;

    case SMS_ALARM_HUMIDITY_HIGH:
        ctx->humidity_high_count++;
        break;

    case SMS_ALARM_TEMP_CHANGE:
        ctx->temp_change_count++;
        break;

    default:
        break;
    }
}

static int forward_alert_to_logger(AlertThreadContext *ctx,
                                   const AlarmMessage *alarm)
{
    LogMessage message;

    memset(&message, 0, sizeof(message));

    message.type = SMS_LOG_ALERT;
    message.timestamp_ms = alarm->timestamp_ms;
    message.alarm_type = alarm->type;
    message.value = alarm->value;
    message.threshold = alarm->threshold;

    while (mq_send(ctx->log_queue,
                   (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno == EINTR)
            continue;

        return errno;
    }

    return 0;
}

static int notify_logger_stop(AlertThreadContext *ctx)
{
    LogMessage message;

    memset(&message, 0, sizeof(message));
    message.type = SMS_LOG_STOP;

    while (mq_send(ctx->log_queue,
                   (const char *)&message,
                   sizeof(message), 0) < 0) {
        if (errno == EINTR)
            continue;

        return errno;
    }

    return 0;
}

void *alert_thread_main(void *arg)
{
    AlertThreadContext *ctx = arg;

    if (ctx == NULL) {
        return NULL;
    }

    ctx->error_code = 0;

    for (;;) {
        AlarmMessage alarm;
        ssize_t received;

        received = mq_receive(ctx->alarm_queue,
                              (char *)&alarm,
                              sizeof(alarm),
                              NULL);

        if (received < 0) {
            if (errno == EINTR)
                continue;

            ctx->error_code = errno;
            perror("Alert: mq_receive");
            break;
        }

        if ((size_t)received != sizeof(alarm)) {
            ctx->error_code = EBADMSG;
            fprintf(stderr, "Alert: invalid message size\n");
            break;
        }

        if (alarm.type == SMS_ALARM_STOP) {
            break;
        }

        if (alarm.type < SMS_ALARM_TEMP_HIGH ||
            alarm.type > SMS_ALARM_TEMP_CHANGE) {
            ctx->error_code = EINVAL;
            fprintf(stderr, "Alert: unknown alarm type\n");
            break;
        }

        count_alarm(ctx, alarm.type);

        fprintf(stderr,
                "[ALERT][%llu] %s: VALUE: %.2f THRESHOLD: %.2f\n",
                (unsigned long long)alarm.timestamp_ms,
                get_alarm_name(alarm.type),
                alarm.value,
                alarm.threshold);

        if (forward_alert_to_logger(ctx, &alarm) != 0) {
            ctx->error_code = errno;
            perror("Alert: forward to logger");
            break;
        }
    }

    if (notify_logger_stop(ctx) != 0 && ctx->error_code == 0) {
        ctx->error_code = errno;
        perror("Alert: notify logger shutdown");
    }

    fprintf(stderr,
            "[ALERT] Summary: TEMP_HIGH=%llu, "
            "HUMIDITY_HIGH=%llu, TEMP_CHANGE=%llu\n",
            (unsigned long long)ctx->temp_high_count,
            (unsigned long long)ctx->humidity_high_count,
            (unsigned long long)ctx->temp_change_count);

    return NULL;
}
