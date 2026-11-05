#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

int main(void)
{
    const char *queue_name = "/asm02_test_queue";
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = 128,
        .mq_curmsgs = 0
    };

    mqd_t mq;
    char buffer[128];

    printf("===== POSIX MESSAGE QUEUE TEST =====\n");

    mq_unlink(queue_name);

    mq = mq_open(queue_name, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("[FAIL] mq_open");
        return 1;
    }
    printf("[PASS] mq_open\n");

    const char *message = "ASM02 POSIX MQ works";

    if (mq_send(mq, message, strlen(message) + 1, 0) == -1) {
        perror("[FAIL] mq_send");
        mq_close(mq);
        mq_unlink(queue_name);
        return 1;
    }
    printf("[PASS] mq_send\n");

    ssize_t bytes = mq_receive(mq, buffer, sizeof(buffer), NULL);
    if (bytes == -1) {
        perror("[FAIL] mq_receive");
        mq_close(mq);
        mq_unlink(queue_name);
        return 1;
    }

    printf("[PASS] mq_receive: %s\n", buffer);

    if (strcmp(buffer, message) != 0) {
        printf("[FAIL] Message mismatch\n");
        mq_close(mq);
        mq_unlink(queue_name);
        return 1;
    }

    printf("[PASS] Message integrity\n");

    mq_close(mq);
    mq_unlink(queue_name);

    printf("[PASS] mq_close and mq_unlink\n");
    printf("===== ALL POSIX MQ TESTS PASSED =====\n");

    return 0;
}
