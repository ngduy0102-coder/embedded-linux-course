#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

#ifdef __KERNEL__
#include <linux/types.h>
typedef __u32 sms_u32;
#else
#include <stdint.h>
typedef uint32_t sms_u32;
#endif

#define SMS_SENSOR_DEVICE "/dev/sms_sensor"
#define SMS_SENSOR_PROC   "/proc/sms_stats"

#define SMS_IOCTL_MAGIC  'S'
#define SMS_SET_INTERVAL _IOW(SMS_IOCTL_MAGIC, 1, sms_u32)

#define SMS_DEFAULT_INTERVAL_MS 100
#define SMS_MIN_INTERVAL_MS     10

#endif
