#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "asgn1_ioctl.h"

#define CHECK(call) do { \
    if ((call) < 0) { \
        perror(#call); \
        close(fd); \
        return 1; \
    } \
} while (0)

int main(void)
{
    int fd;
    int mode;
    int invalid_mode = 2;
    char version[64] = {0};
    char buffer[128] = {0};
    ssize_t n;
    struct asgn1_stats stats;

    printf("===== TASK 1 IOCTL DRIVER TEST =====\n");

    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/asgn1");
        return 1;
    }
    printf("[PASS] Device opened successfully\n");

    CHECK(ioctl(fd, ASGN1_GET_VERSION, version));
    printf("[PASS] Driver version: %s\n", version);

    /* Test Mode 0: Kernel mode */
    printf("\n[TEST 1] Kernel mode\n");
    mode = 0;
    CHECK(ioctl(fd, ASGN1_SET_MODE, &mode));

    n = write(fd, "Hello Kernel", 12);
    if (n < 0) {
        perror("write Kernel mode");
        close(fd);
        return 1;
    }

    close(fd);
    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("reopen /dev/asgn1");
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("read Kernel mode");
        close(fd);
        return 1;
    }
    buffer[n] = '\0';

    printf("Read data: [%s]\n", buffer);
    if (strcmp(buffer, "[KERNEL]Hello Kernel") == 0)
        printf("[PASS] Kernel mode prefix verified\n");
    else {
        printf("[FAIL] Kernel mode data mismatch\n");
        close(fd);
        return 1;
    }

    /* Test Mode 1: Echo mode */
    printf("\n[TEST 2] Echo mode\n");
    mode = 1;
    CHECK(ioctl(fd, ASGN1_SET_MODE, &mode));

    n = write(fd, "Hello ASM1", 10);
    if (n < 0) {
        perror("write Echo mode");
        close(fd);
        return 1;
    }

    close(fd);
    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("reopen /dev/asgn1");
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("read Echo mode");
        close(fd);
        return 1;
    }
    buffer[n] = '\0';

    printf("Read data: [%s]\n", buffer);
    if (strcmp(buffer, "Hello ASM1") == 0)
        printf("[PASS] Echo mode verified\n");
    else {
        printf("[FAIL] Echo mode data mismatch\n");
        close(fd);
        return 1;
    }

    /* Test statistics */
    printf("\n[TEST 3] GET_STATS\n");
    CHECK(ioctl(fd, ASGN1_GET_STATS, &stats));

    printf("open_count      = %d\n", stats.open_count);
    printf("write_count     = %d\n", stats.write_count);
    printf("read_count      = %d\n", stats.read_count);
    printf("buffer_len      = %d\n", stats.buffer_len);
    printf("last_write_size = %d\n", stats.last_write_size);

    /* Test invalid mode */
    printf("\n[TEST 4] Invalid mode\n");
    errno = 0;
    if (ioctl(fd, ASGN1_SET_MODE, &invalid_mode) < 0 &&
        errno == EINVAL)
        printf("[PASS] Invalid mode rejected: %s\n", strerror(errno));
    else {
        printf("[FAIL] Invalid mode was not rejected correctly\n");
        close(fd);
        return 1;
    }

    /* Test reset and read-after-reset */
    printf("\n[TEST 5] RESET_BUFFER and EOF\n");
    CHECK(ioctl(fd, ASGN1_RESET_BUFFER));
    printf("[PASS] Buffer reset successfully\n");

    CHECK(ioctl(fd, ASGN1_GET_STATS, &stats));
    printf("write_count     = %d\n", stats.write_count);
    printf("read_count      = %d\n", stats.read_count);
    printf("buffer_len      = %d\n", stats.buffer_len);
    printf("last_write_size = %d\n", stats.last_write_size);

    close(fd);
    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("reopen after reset");
        return 1;
    }

    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n == 0)
        printf("[PASS] Read after reset returned EOF (0 bytes)\n");
    else {
        printf("[FAIL] Expected EOF, read returned %d\n", (int)n);
        close(fd);
        return 1;
    }

    /* Test invalid ioctl */
    printf("\n[TEST 6] Invalid ioctl\n");
    errno = 0;
    if (ioctl(fd, 0x1234) < 0 && errno == ENOTTY)
        printf("[PASS] Invalid ioctl rejected: %s\n", strerror(errno));
    else {
        printf("[FAIL] Invalid ioctl was not rejected correctly\n");
        close(fd);
        return 1;
    }

    close(fd);
    printf("\n===== ALL TASK 1 TESTS PASSED =====\n");
    return 0;
}
