#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Maximum number of samples stored in the circular buffer */
#define SMS_BUFFER_CAPACITY 64

/* POSIX message queue names */
#define SMS_ALARM_QUEUE "/alarm_queue"
#define SMS_LOG_QUEUE   "/log_queue"

/* POSIX message queue configuration */
#define SMS_QUEUE_MAX_MESSAGES 10

/* Sensor sample */
typedef struct {
    uint64_t timestamp_ms;
    float temperature;
    float humidity;
} SensorData;

/* Circular buffer statistics */
typedef struct {
    size_t current_size;
    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;
} BufferStats;

/* Alarm types */
typedef enum {
    SMS_ALARM_TEMP_HIGH = 1,
    SMS_ALARM_HUMIDITY_HIGH,
    SMS_ALARM_TEMP_CHANGE,
    SMS_ALARM_STOP
} AlarmType;

/* Message sent from Processor Thread to Alert Thread */
typedef struct {
    AlarmType type;
    uint64_t timestamp_ms;
    float value;
    float threshold;
} AlarmMessage;

/* Log message types */
typedef enum {
    SMS_LOG_DATA = 1,
    SMS_LOG_ALERT,
    SMS_LOG_STOP
} LogMessageType;

/* Message sent from Processor/Alert Threads to Logger Thread */
typedef struct {
    LogMessageType type;
    uint64_t timestamp_ms;

    float temperature;
    float humidity;

    float avg_temperature;
    float avg_humidity;

    AlarmType alarm_type;
    float value;
    float threshold;
} LogMessage;

#endif /* SMS_COMMON_H */
