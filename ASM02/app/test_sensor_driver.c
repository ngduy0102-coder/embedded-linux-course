#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

#include "../driver/sms_sensor.h"

int main(void)
{
    int fd;
    uint32_t interval = 500;
    char buffer[128];
    ssize_t n;
    struct timespec delay = {0, 600000000L};

    printf("===== SENSOR DRIVER TEST =====\n");

    fd = open(SMS_SENSOR_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("open sensor");
        return 1;
    }

    printf("[PASS] Open sensor device\n");

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        perror("ioctl SMS_SET_INTERVAL");
        close(fd);
        return 1;
    }

    printf("[PASS] Set interval = %u ms\n", interval);

    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("first read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    printf("[PASS] First read: %s", buffer);

    errno = 0;
    n = read(fd, buffer, sizeof(buffer) - 1);

    if (n < 0 && errno == EAGAIN) {
        printf("[PASS] Immediate read returned EAGAIN\n");
    } else {
        printf("[FAIL] Expected EAGAIN, got n=%zd errno=%d\n",
               n, errno);
    }

    nanosleep(&delay, NULL);

    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("read after interval");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    printf("[PASS] Read after 600 ms: %s", buffer);

    close(fd);
    printf("[PASS] Close sensor device\n");

    printf("\n===== PROC STATISTICS =====\n");

    fd = open(SMS_SENSOR_PROC, O_RDONLY);
    if (fd >= 0) {
        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        }
        close(fd);
    } else {
        perror("open proc stats");
    }

    return 0;
}
