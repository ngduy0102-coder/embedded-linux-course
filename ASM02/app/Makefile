CC = gcc
CROSS_COMPILE = arm-linux-gnueabihf-

CFLAGS = -Wall -Wextra -Werror -std=c11 -pthread -I.
LDFLAGS = -pthread -lrt

COMMON_SRCS = main.c \
              processor_thread.c \
              alert_thread.c \
              logger_thread.c \
              circular_buffer.c

HOST_SRCS = $(COMMON_SRCS) sensor_thread_host.c
ARM_SRCS = $(COMMON_SRCS) sensor_thread.c

.PHONY: all host arm clean

all: host arm

host:
	$(CC) $(CFLAGS) $(HOST_SRCS) -o sms_app_host $(LDFLAGS)

arm:
	$(CROSS_COMPILE)gcc -static $(CFLAGS) $(ARM_SRCS) -o sms_app_arm $(LDFLAGS)

clean:
	rm -f sms_app_host sms_app_arm
