#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "asgn1_ioctl.h"

int main(void)
{
    int fd;

    fd = open("/dev/asgn1", O_RDWR);

    if (fd < 0) {
        perror("open /dev/asgn1");
        return 1;
    }

    printf("Device opened successfully\n");

    char version[64];

    if (ioctl(fd, ASGN1_GET_VERSION, version) < 0) {
        perror("ASGN1_GET_VERSION");
        close(fd);
        return 1;
    }

    printf("Driver version: %s\n", version);

    int mode = 1;

    if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0) {
        perror("ASGN1_SET_MODE");
        close(fd);
        return 1;
    }

    printf("Driver mode set to: %d\n", mode);

    const char *msg = "Hello ASM1";
    char buffer[128];
    ssize_t n;

    n = write(fd, msg, strlen(msg));

    if (n < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Write returned: %d\n", (int)n);

    lseek(fd, 0, SEEK_SET);

    n = read(fd, buffer, sizeof(buffer) - 1);

    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';

    printf("Read returned: %d\n", (int)n);
    printf("Read data: %s\n", buffer);

    struct asgn1_stats stats;

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
        perror("ASGN1_GET_STATS");
        close(fd);
        return 1;
    }

    printf("Stats:\n");
    printf("  open_count      = %d\n", stats.open_count);
    printf("  write_count     = %d\n", stats.write_count);
    printf("  read_count      = %d\n", stats.read_count);
    printf("  buffer_len      = %d\n", stats.buffer_len);
    printf("  last_write_size = %d\n", stats.last_write_size);

    if (ioctl(fd, ASGN1_RESET_BUFFER) < 0) {
        perror("ASGN1_RESET_BUFFER");
        close(fd);
        return 1;
    }

    printf("Buffer reset successfully\n");

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
        perror("ASGN1_GET_STATS after reset");
        close(fd);
        return 1;
    }

    printf("Stats after reset:\n");
    printf("  write_count     = %d\n", stats.write_count);
    printf("  read_count      = %d\n", stats.read_count);
    printf("  buffer_len      = %d\n", stats.buffer_len);
    printf("  last_write_size = %d\n", stats.last_write_size);

    if (ioctl(fd, 0x1234) < 0) {
        printf("Invalid ioctl correctly rejected: %s\n",
               strerror(errno));
    } else {
        printf("ERROR: invalid ioctl was accepted\n");
    }

    close(fd);

    return 0;
}
