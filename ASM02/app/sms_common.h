#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Maximum number of samples stored in the circular buffer */
#define SMS_BUFFER_CAPACITY 64

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

#endif /* SMS_COMMON_H */
