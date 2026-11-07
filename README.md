# EMBEDDED LINUX COURSE
## LAB01–LAB02-LAB03, ASM01–ASM02 — Embedded Linux

This document summarizes the development environment, implementation, architecture, build procedures, testing, and recorded results of LAB01, LAB02, LAB03, ASM01, and ASM02.

This repository-level README brings together the five practical assignments. Individual LAB and ASM directories retain their own README files and supporting evidence.

---

# TABLE OF CONTENTS

- [1. Development Environment](#1-development-environment)
- [2. Repository Structure](#2-repository-structure)
- [3. LAB01 - Embedded Linux Environment and Kernel Build](#3-lab01---embedded-linux-environment-and-kernel-build)
  - [3.1 Overview](#31-overview)
  - [3.2 Toolchain and Target Platform](#32-toolchain-and-target-platform)
  - [3.3 Kernel, Root Filesystem and QEMU](#33-kernel-root-filesystem-and-qemu)
- [4. LAB02 - Character Device Driver and NAND Filesystem](#4-lab02---character-device-driver-and-nand-filesystem)
  - [4.1 Character Device Driver](#41-character-device-driver)
  - [4.2 Procfs and Sysfs Interfaces](#42-procfs-and-sysfs-interfaces)
  - [4.3 NAND Simulation and JFFS2](#43-nand-simulation-and-jffs2)
  - [4.4 Boot Integration and Testing](#44-boot-integration-and-testing)
- [5. LAB03 - Debugging, Profiling and Real-Time Analysis](#5-lab03---debugging-profiling-and-real-time-analysis)
  - [5.1 GDB Debugging](#51-gdb-debugging)
  - [5.2 Memory Analysis with Valgrind](#52-memory-analysis-with-valgrind)
  - [5.3 System Call and Performance Analysis](#53-system-call-and-performance-analysis)
  - [5.4 Real-Time Latency Analysis](#54-real-time-latency-analysis)
- [6. ASM01 - IOCTL and Miscellaneous Device Drivers](#6-asm01---ioctl-and-miscellaneous-device-drivers)
  - [6.1 Overview](#61-overview)
  - [6.2 Task 1 - IOCTL Character Driver](#62-task-1---ioctl-character-driver)
  - [6.3 Task 2 - Miscellaneous Counter Driver](#63-task-2---miscellaneous-counter-driver)
  - [6.4 Build and Test ASM01](#64-build-and-test-asm01)
  - [6.5 ASM01 Test Results](#65-asm01-test-results)
- [7. ASM02 - Sensor Monitoring and Real-Time Application](#7-asm02---sensor-monitoring-and-real-time-application)
  - [7.1 Overview](#71-overview)
  - [7.2 Task 1 - Sensor Driver and Multithreaded Application](#72-task-1---sensor-driver-and-multithreaded-application)
  - [7.3 Task 2 - POSIX Message Queues and Memory Analysis](#73-task-2---posix-message-queues-and-memory-analysis)
  - [7.4 Task 3 - Debugging and Performance Analysis](#74-task-3---debugging-and-performance-analysis)
  - [7.5 Task 4 - Real-Time Scheduling and Jitter Benchmark](#75-task-4---real-time-scheduling-and-jitter-benchmark)
  - [7.6 Build ASM02](#76-build-asm02)
  - [7.7 ASM02 Test Evidence](#77-asm02-test-evidence)
- [8. Technical Notes and Measurement Limitations](#8-technical-notes-and-measurement-limitations)

# 1. DEVELOPMENT ENVIRONMENT

The assignments were developed on an Ubuntu x86_64 host and cross-compiled for an ARM target. ARM kernel modules and applications were tested using QEMU's Versatile Express A9 platform.

| Component | Configuration |
|---|---|
| Host operating system | Ubuntu Linux x86_64 |
| Linux kernel source | Linux 5.15 |
| Target architecture | ARMv7-A |
| QEMU machine | `vexpress-a9` |
| Emulated CPU | Cortex-A9 |
| Emulated memory | 512 MB |
| Cross compiler | `arm-linux-gnueabihf-gcc` |
| Kernel build architecture | `ARCH=arm` |
| Kernel module build system | Kbuild |
| Application language | C |
| Build system | GNU Make |
| Concurrency | POSIX Threads |
| Inter-process/thread communication | POSIX Message Queues |

The kernel source used by the assignment Makefiles is located at:

```text
../../LAB01/kernel/linux-5.15
```

The ARM cross-compilation prefix is:

```text
arm-linux-gnueabihf-
```

---

# 2. REPOSITORY STRUCTURE

The repository contains five main labs and assignments.
Each directory stores source code, configuration, test evidence,
and relevant build artifacts.

```text
embedded_linux_course/
│
├── README.md
├── .gitignore
│
├── LAB01/
│   ├── README.md
│   ├── configs/
│   ├── evidence/
│   ├── kernel/
│   ├── rootfs/
│   ├── uboot/
│   └── output/
│
├── LAB02/
│   ├── README.md
│   ├── driver/
│   ├── rootfs/
│   ├── jffs2_data/
│   ├── jffs2_host_data/
│   ├── evidence/
│   └── output/
│
├── LAB03/
│   ├── README.md
│   ├── apps/
│   ├── scripts/
│   ├── rootfs/
│   ├── rt_kernel/
│   ├── evidence/
│   ├── logs/
│   └── output/
│
├── ASM01/
│   ├── .gitignore
│   ├── task1_ioctl/
│   │   ├── Makefile
│   │   ├── asgn1_driver.c
│   │   ├── asgn1_ioctl.h
│   │   └── test_asgn1_ioctl.c
│   ├── task2_misc/
│   │   ├── Makefile
│   │   ├── counter_driver.c
│   │   └── test_counter.sh
│   └── evidence/
│       ├── task1/
│       └── task2/
│
└── ASM02/
    ├── driver/
    │   ├── Makefile
    │   ├── sms_sensor.h
    │   └── sms_sensor_driver.c
    ├── app/
    │   ├── Makefile
    │   ├── main.c
    │   ├── sms_common.h
    │   ├── circular_buffer.c
    │   ├── circular_buffer.h
    │   ├── sensor_thread.c
    │   ├── sensor_thread.h
    │   ├── sensor_thread_host.c
    │   ├── processor_thread.c
    │   ├── processor_thread.h
    │   ├── alert_thread.c
    │   ├── alert_thread.h
    │   ├── logger_thread.c
    │   ├── logger_thread.h
    │   ├── realtime_benchmark.c
    │   └── test_*.c
    ├── evidence/
    │   ├── task1_application/
    │   ├── task1_driver/
    │   ├── task2_application/
    │   ├── task2_memory/
    │   ├── task3_debug/
    │   └── task4_realtime/
    ├── logs/
    └── output/
```

# 3. LAB01 - EMBEDDED LINUX ENVIRONMENT AND KERNEL BUILD

## 3.1 Overview

LAB01 establishes the Embedded Linux development workspace and prepares a Linux system for an ARM-based target. The work covers the kernel source tree, cross-compilation environment, root filesystem, Initramfs, and QEMU boot process.

The lab verifies that a custom Linux kernel can be built and booted in an emulated ARM environment.

## 3.2 Toolchain and Target Platform

| Component | Configuration |
|---|---|
| Host operating system | Ubuntu Linux x86_64 |
| Cross compiler | `arm-linux-gnueabihf-gcc-11` |
| Emulator | `qemu-system-arm` |
| Target architecture | ARMv7-A |
| CPU model | Cortex-A9 |
| QEMU machine | `vexpress-a9` |
| Linux kernel | 5.15.0 |

The ARM cross compiler is used to build target binaries on the x86_64 host. QEMU provides an emulated ARM platform for boot and runtime verification.

## 3.3 Kernel, Root Filesystem and QEMU

The completed workflow includes:

- Preparing the workspace and Git repository.
- Preparing and configuring the Linux 5.15 kernel source.
- Building the ARM kernel image.
- Building BusyBox for the target environment.
- Creating the root filesystem and Initramfs.
- Preparing the QEMU machine and device tree.
- Booting the kernel in QEMU and verifying the userspace environment.

The successful QEMU boot confirms that the kernel image, device tree, and initial userspace filesystem work together in the configured emulated environment.

---

# 4. LAB02 - CHARACTER DEVICE DRIVER AND NAND FILESYSTEM

## 4.1 Character Device Driver

LAB02 implements a Linux character device driver exposed through `/dev/lab2`.

The driver uses major number `240` and implements the standard file operations:

- `open()`
- `read()`
- `write()`
- `release()`

The implementation transfers data between kernel space and user space using the appropriate user-copy APIs. A mutex protects shared driver data against concurrent access.

## 4.2 Procfs and Sysfs Interfaces

The driver exposes runtime information through procfs and sysfs.

| Interface | Purpose |
|---|---|
| `/proc/lab2_info` | Displays driver information and runtime status |
| `buffer_len` | Reports the current buffer length |
| `open_count` | Reports the number of open operations |
| `last_data` | Exposes the most recently stored data |

These interfaces provide a way to inspect driver state without adding custom application-specific IOCTL commands.

## 4.3 NAND Simulation and JFFS2

The lab simulates NAND flash storage using the Linux `nandsim` module and uses JFFS2 as the flash filesystem.

The workflow covers:

- Loading and configuring the NAND simulator.
- Preparing an MTD partition.
- Erasing the simulated flash using `flash_erase`.
- Creating a JFFS2 image with `mkfs.jffs2`.
- Writing the image using `nandwrite`.
- Mounting the filesystem and checking stored files.
- Inspecting NAND contents and filesystem behavior.

The NAND dump was also inspected at byte level. The observed JFFS2 magic value `85 19` provides evidence that JFFS2-formatted data was written to the simulated flash.

Persistence was verified within the same NAND simulator session. This should not be interpreted as proof of persistence across every possible simulator reset or reinitialization scenario.

## 4.4 Boot Integration and Testing

The driver is integrated into the target startup process using BusyBox init configuration, including `inittab` and `rcS`.

The boot script loads the driver and prepares the device node so that the device is available after startup.

The lab was tested in the QEMU `vexpress-a9` environment using the ARMv7-A Cortex-A9 target and Linux 5.15. The recorded D5 runtime checks completed successfully.

---

# 5. LAB03 - DEBUGGING, PROFILING AND REAL-TIME ANALYSIS

## 5.1 GDB Debugging

LAB03 uses GDB and remote debugging to inspect application execution and investigate common C programming errors.

The debugging exercises cover:

- Array out-of-bounds access.
- Use-after-free.
- Integer overflow.
- Stepping through program execution.
- Inspecting variables, memory, and program state.

The exercises demonstrate how runtime debugging can help locate memory and logic errors that may not be obvious from source-code inspection alone.

## 5.2 Memory Analysis with Valgrind

Valgrind is used to detect memory-management problems, including memory leaks and invalid memory operations.

The lab compares problematic and corrected application behavior. It also examines memory usage for ARM-targeted applications.

Memory measurements must be interpreted in context: virtual memory size, resident memory, and heap allocation describe different aspects of process memory usage.

## 5.3 System Call and Performance Analysis

The lab uses `strace` to inspect system calls and `perf` to profile application execution.

Recorded analysis includes:

- System-call tracing and call counts.
- Identification of frequently used system calls.
- CPU performance profiling.
- Comparison of host and ARM-targeted application behavior.
- Cross-compilation and execution in QEMU.

In the recorded `strace` analysis, 127 system calls were observed. `write()` accounted for 28.00% of the measured system-call time in that run. This percentage describes the recorded workload and is not a universal performance characteristic.

## 5.4 Real-Time Latency Analysis

The real-time analysis uses `cyclictest` under CPU, I/O, and virtual-memory stress generated by `stress-ng`.

Host test configuration:

| Item | Configuration |
|---|---|
| Host kernel | Linux `7.0.0-42-generic` |
| Preemption model | `PREEMPT_DYNAMIC` |
| PREEMPT_RT | Disabled |
| Cyclictest threads | 4 |
| Scheduling policy | `SCHED_FIFO` |
| Iterations per thread | 10,000 |
| Stress workload | 12 CPU, 4 I/O, 2 VM workers |

Recorded latency results:

| Thread | Minimum (µs) | Average (µs) | Maximum (µs) |
|---|---:|---:|---:|
| 0 | 2 | 3 | 56 |
| 1 | 2 | 3 | 266 |
| 2 | 2 | 3 | 212 |
| 3 | 2 | 3 | 115 |

The histogram recorded 39,998 samples, with approximately 99.98% below 50 µs. Two histogram overflows were reported. The maximum observed latency was 266 µs, so the histogram's displayed range must not be mistaken for the absolute maximum.

The host kernel did not have PREEMPT_RT enabled. Therefore, these measurements are baseline host results and do not constitute a direct PREEMPT_RT versus non-RT comparison. Any PREEMPT_RT discussion is theoretical unless a separate RT-kernel test is performed.

The detailed D1-D5 evidence is stored in `LAB03/evidence/D1_D5_realtime_analysis.txt`. The latency histogram is stored in `LAB03/logs/latency_histogram.png`.

---

# 6. ASM01 - IOCTL AND MISCELLANEOUS DEVICE DRIVERS

## 6.1 Overview

ASM01 focuses on Linux kernel module development and communication between kernel space and user space.

The assignment contains two driver implementations:

| Task | Driver | Main concepts |
|---|---|---|
| Task 1 | IOCTL character device | Character device registration, file operations, IOCTL commands, user/kernel data transfer |
| Task 2 | Miscellaneous counter device | miscdevice, atomic operations, procfs, input validation |

Both drivers were built for the ARM target using the Linux kernel build system and tested in the QEMU environment.

## 6.2 Task 1 - IOCTL Character Driver

### Objective

Implement a character device driver that supports two operating modes and provides a set of IOCTL commands for controlling and querying the driver.

### Source files

```text
ASM01/task1_ioctl/
├── Makefile
├── asgn1_driver.c
├── asgn1_ioctl.h
└── test_asgn1_ioctl.c
```

### Driver configuration

| Property | Value |
|---|---|
| Driver/device name | `asgn1` |
| Device node | `/dev/asgn1` |
| Major number | 241 |
| Minor number | 0 |
| Buffer size | 1024 bytes |
| Module version | 1.0 |

### Supported operations

The character driver implements the standard file operations required for user-space interaction, including opening, reading, writing, and releasing the device.

It supports two operating modes:

**1. Kernel mode**

Written data is processed by the driver and returned with the `[KERNEL]` prefix.

Example:

```text
Input:  Hello Kernel
Output: [KERNEL]Hello Kernel
```

**2. Echo mode**

The driver returns the original written data.

Example:

```text
Input:  Hello ASM1
Output: Hello ASM1
```

### IOCTL interface

The driver provides the following control operations:

| IOCTL operation | Purpose |
|---|---|
| `RESET_BUFFER` | Clear the driver's internal buffer |
| `GET_STATS` | Retrieve driver statistics |
| `SET_MODE` | Change the operating mode |
| `GET_VERSION` | Retrieve driver version information |

The implementation also validates IOCTL arguments and rejects unsupported commands.

### Test coverage

The user-space test program verifies:

1. Opening `/dev/asgn1`.
2. Retrieving driver version using `GET_VERSION`.
3. Switching to kernel mode.
4. Writing and reading data in kernel mode.
5. Switching to echo mode.
6. Writing and reading data in echo mode.
7. Retrieving statistics using `GET_STATS`.
8. Rejecting an invalid mode with `EINVAL`.
9. Resetting the internal buffer.
10. Confirming EOF after buffer reset.
11. Rejecting an unsupported IOCTL with `ENOTTY`.

### Build Task 1

Run on the Ubuntu host:

```bash
cd ~/embedded_linux_course/ASM01/task1_ioctl
make
```

Clean generated build files:

```bash
make clean
```

The Makefile uses the Linux 5.15 kernel source under `LAB01/kernel/linux-5.15`, with `ARCH=arm` and the `arm-linux-gnueabihf-` cross compiler.

### Test evidence

```text
ASM01/evidence/task1/task1_test.log
```

Recorded test output ends with:

```text
===== ALL TASK 1 TESTS PASSED =====
```

**Task 1 result: PASS.**

## 6.3 Task 2 - Miscellaneous Counter Driver

### Objective

Implement a Linux miscellaneous character device that maintains a counter and exposes driver information through procfs.

### Source files

```text
ASM01/task2_misc/
├── Makefile
├── counter_driver.c
└── test_counter.sh
```

### Driver interfaces

| Interface | Description |
|---|---|
| Device node | `/dev/counter` |
| procfs entry | `/proc/counter_info` |

### Main functionality

- Register a miscellaneous device.
- Maintain the counter using atomic operations.
- Track the total number of read operations.
- Track the total number of write operations.
- Return the current counter value when the device is read.
- Increment the counter after a successful read.
- Set the counter from a valid non-negative integer supplied by a write.
- Reject nonnumeric input.
- Reject negative values.
- Expose counter information and operation statistics through procfs.

### Example behavior

If the initial counter value is zero:

```text
Read 1  -> returns 0; counter becomes 1
Read 2  -> returns 1; counter becomes 2
Write 10 -> counter becomes 10
Write 0  -> counter becomes 0
```

The exact behavior is implemented by the driver's read and write handlers.

### Build Task 2

```bash
cd ~/embedded_linux_course/ASM01/task2_misc
make
```

Clean generated build files:

```bash
make clean
```

### Test coverage

The test script checks:

1. Driver module loading.
2. Device-node availability.
3. Initial counter value.
4. Counter increment after reads.
5. Setting the counter through writes.
6. Resetting the counter.
7. Rejection of nonnumeric input.
8. Rejection of negative input.
9. Counter and operation statistics.
10. Driver module unloading.

### Test evidence

```text
ASM01/evidence/task2/task2_test.log
```

The recorded log ends with:

```text
===== ALL TASK 2 TESTS COMPLETED =====
```

The log confirms the individual operations, invalid-input checks, statistics checks, and module unload step completed. The terminal banner says `COMPLETED`, not `ALL TESTS PASSED`, so this README preserves that distinction.

## 6.4 Build and Test ASM01

The two tasks are built independently:

```bash
# Task 1
cd ~/embedded_linux_course/ASM01/task1_ioctl
make

# Task 2
cd ~/embedded_linux_course/ASM01/task2_misc
make
```

The test programs/scripts are intended to run in the ARM Linux test environment with the corresponding driver loaded and device nodes available.

For a complete test, prepare the target kernel and root filesystem, boot the ARM QEMU system, copy or include the generated module and test application, then load and test the module inside the guest.

## 6.5 ASM01 Test Results

| Test area | Recorded result |
|---|---|
| Task 1 device open | PASS |
| Task 1 GET_VERSION | PASS |
| Task 1 kernel mode | PASS |
| Task 1 echo mode | PASS |
| Task 1 GET_STATS | PASS |
| Task 1 invalid mode handling | PASS |
| Task 1 RESET_BUFFER and EOF | PASS |
| Task 1 invalid IOCTL handling | PASS |
| Task 1 overall test banner | ALL TASK 1 TESTS PASSED |
| Task 2 read/write behavior | Completed in recorded test |
| Task 2 invalid input rejection | Completed in recorded test |
| Task 2 statistics checks | Completed in recorded test |
| Task 2 module unload | Completed in recorded test |

Evidence files:

- `ASM01/evidence/task1/task1_test.log`
- `ASM01/evidence/task2/task2_test.log`

---

# 7. ASM02 - SENSOR MONITORING AND REAL-TIME APPLICATION

## 7.1 Overview

ASM02 develops a sensor monitoring system using a Linux kernel driver and a multithreaded user-space application.

The system demonstrates:

- Linux character device driver development.
- IOCTL-based driver configuration.
- Kernel-to-user-space data transfer.
- POSIX Threads.
- Thread synchronization.
- Circular buffer implementation.
- POSIX Message Queues.
- Sensor data processing and alert generation.
- Logging and log rotation.
- GDB debugging.
- `strace` system call analysis.
- `perf` profiling.
- Memory and runtime analysis.
- Real-time scheduling with `SCHED_FIFO`.
- Jitter benchmarking.

### High-level architecture

```text
                 ARM Linux / QEMU
                        |
                        v
             +----------------------+
             |   SMS Sensor Driver  |
             |  /dev/sms_sensor     |
             |  /proc/sms_stats     |
             +----------------------+
                        |
                 Sensor data read
                        |
                        v
             +----------------------+
             |    Sensor Thread     |
             +----------------------+
                        |
                        v
             +----------------------+
             |    Circular Buffer   |
             | Capacity: 64 samples |
             +----------------------+
                        |
                        v
             +----------------------+
             |   Processor Thread  |
             | Rolling average     |
             | Threshold checking  |
             +----------------------+
                    /       \
                   /         \
                  v           v
          +-------------+  +-------------+
          | Alarm Queue |  |  Log Queue  |
          +-------------+  +-------------+
                  |           |
                  v           v
          +-------------+  +-------------+
          | Alert Thread|  | Logger Thread|
          +-------------+  +-------------+
                              |
                              v
                         /tmp/sms.log
```

The application uses separate threads for sensor acquisition, processing, alert handling, and logging.

## 7.2 Task 1 - Sensor Driver and Multithreaded Application

### A. Sensor kernel driver

Main source files:

```text
ASM02/driver/
├── Makefile
├── sms_sensor.h
└── sms_sensor_driver.c
```

### Driver interfaces

| Interface | Purpose |
|---|---|
| `/dev/sms_sensor` | Character device used to read sensor samples |
| `/proc/sms_stats` | Exposes sensor driver statistics |
| `SMS_SET_INTERVAL` | IOCTL command to configure the sampling interval |

### Sensor data format

The driver returns sensor samples in CSV format:

```text
<timestamp_ms>,<temperature>,<humidity>
```

Recorded simulated sensor ranges:

| Measurement | Range |
|---|---|
| Temperature | 15-45 °C |
| Humidity | 30-90% |

The driver supports a configurable sampling interval. If user space attempts to read again before the configured interval has elapsed, the driver returns `EAGAIN`.

The driver tracks successful reads through its statistics interface.

### Driver test

The QEMU driver test verifies:

1. Opening `/dev/sms_sensor`.
2. Setting the sampling interval to 500 ms.
3. Reading a valid CSV sensor sample.
4. Attempting an immediate second read and receiving `EAGAIN`.
5. Waiting and reading again successfully.
6. Closing the device.
7. Checking the read counter.

Recorded evidence:

```text
ASM02/evidence/task1_driver/sensor_driver_qemu_test.txt
```

The test log records successful device access, interval configuration, valid sensor data, expected `EAGAIN` behavior, and successful subsequent read.

### B. Multithreaded user-space application

Main source files:

```text
ASM02/app/
├── main.c
├── sms_common.h
├── circular_buffer.c
├── circular_buffer.h
├── sensor_thread.c
├── sensor_thread.h
├── sensor_thread_host.c
├── processor_thread.c
├── processor_thread.h
├── alert_thread.c
├── alert_thread.h
├── logger_thread.c
├── logger_thread.h
└── Makefile
```

The application consists of four main worker threads:

| Thread | Responsibility |
|---|---|
| Sensor Thread | Acquire sensor data from the driver |
| Processor Thread | Process samples and evaluate thresholds |
| Alert Thread | Handle alert messages and report abnormal conditions |
| Logger Thread | Receive log messages and write them to a log file |

### Circular buffer

The application uses a circular buffer with:

- Capacity of 64 sensor samples.
- Mutex-based synchronization.
- Condition-variable signaling.
- Producer-consumer coordination between sensor acquisition and processing.

The buffer prevents the sensor and processor threads from accessing shared sample data without synchronization.

### Data processing

The processor evaluates incoming samples and calculates a rolling average over the latest 10 samples.

The application supports configurable alert thresholds for temperature and humidity, and detects temperature changes greater than 5 °C.

### Alert and logging pipeline

POSIX Message Queues are used to pass messages to the alert and logging components.

The logger writes messages to:

```text
/tmp/sms.log
```

The logging implementation includes:

- Periodic flushing, every 5 seconds.
- Log rotation when the log file reaches 1 MB.
- Graceful shutdown handling.

### Command-line options

The application supports the following options:

| Option | Purpose |
|---|---|
| `-i ms` | Configure sensor sampling interval |
| `-t °C` | Configure temperature threshold |
| `-h %` | Configure humidity threshold |
| `-d sec` | Configure application duration |
| `-v` | Enable verbose output |

### Host and ARM builds

Two sensor-thread implementations are provided:

- `sensor_thread.c`: reads from the real sensor character device; used in the ARM target build.
- `sensor_thread_host.c`: host-side sensor simulation; used for host testing and analysis.

This separation allows application logic to be tested on the development host while retaining a driver-based sensor implementation for the ARM target.

## 7.3 Task 2 - POSIX Message Queues and Memory Analysis

### POSIX Message Queues

The Linux kernel configuration enables POSIX Message Queues:

```text
CONFIG_POSIX_MQUEUE=y
```

The application uses two message queues:

| Queue | Purpose |
|---|---|
| `alarm_queue` | Delivers alert messages |
| `log_queue` | Delivers logging messages |

The queue workflow uses standard POSIX message queue operations, including opening, sending, receiving, closing, and unlinking queues.

### QEMU message queue test

The recorded test checks:

1. Opening a POSIX message queue.
2. Sending a message.
3. Receiving the message.
4. Verifying message integrity.
5. Closing the queue.
6. Unlinking the queue.

Evidence:

```text
ASM02/evidence/task2_application/posix_mqueue_qemu_test.txt
```

The recorded test reports successful queue creation, send, receive, message integrity, close, and unlink operations.

### Memory and runtime checks

The assignment includes memory-related checks and process runtime observations, including:

- Process memory snapshots at different execution times.
- Thread count.
- Memory map count.
- Sensor read-count behavior.
- Host-side memory diagnostics.

A recorded 60-second integration check compares the sensor driver's read-count change against the expected sampling activity.

**Measurement caution:** the recorded memory snapshots show a very small `VmRSS` value relative to `VmSize` and `VmData`. These figures should be treated as the values reported by that specific measurement method and environment, not as a complete measure of the application's physical memory footprint.

## 7.4 Task 3 - Debugging and Performance Analysis

### A. GDB multithreaded debugging

GDB was used to inspect application execution and thread behavior.

The recorded debugging work includes:

- Setting a breakpoint in `alert_thread_main`.
- Inspecting the processor thread while it waits on a condition variable.
- Inspecting the logger thread while it waits for a message.
- Observing the alert thread at the breakpoint.
- Checking thread termination during application shutdown.

The recorded analysis reports that all four worker threads exited normally and the inferior exited normally.

Evidence:

```text
ASM02/evidence/task3_debug/gdb_analysis.txt
ASM02/evidence/task3_debug/gdb_alert_debug.txt
```

The alert debug log also records runtime alert categories, including:

- `TEMP_HIGH`
- `HUMIDITY_HIGH`
- `TEMP_CHANGE`

### B. strace system call analysis

`strace` was used to inspect system calls made by the application.

The recorded summary contains 1,278 system calls and 28 reported errors.

Top entries in the recorded syscall-time summary:

| System call | Share of recorded syscall time |
|---|---:|
| `clock_nanosleep` | 38.90% |
| `mq_timedreceive` | 36.19% |
| `futex` | 19.76% |
| `mq_timedsend` | 2.26% |
| `write` | 1.68% |

These results indicate that the observed execution spends a substantial portion of its measured syscall time in timed waits, message queue reception, and thread synchronization.

The reported syscall errors are trace results; they should not automatically be interpreted as application defects without examining the individual calls and their return values.

Evidence:

```text
ASM02/evidence/task3_debug/strace_summary.txt
```

### C. perf profiling

`perf` was used to collect CPU profiling samples and inspect the distribution of sampled execution activity.

Recorded collection:

- 236 CPU-clock samples.
- No lost samples reported.

Recorded inclusive sample distribution:

| Thread/component | Share |
|---|---:|
| Logger | 45.34% |
| Alert | 22.03% |
| Sensor | 11.86% |
| Processor | 8.47% |

The logger had the largest inclusive share in this profiling run.

**Measurement limitation:** this profiling run was performed on the x86_64 host application. It is not an ARM hardware performance measurement. The sample count is also limited, so the percentages should be treated as observations from this run rather than universal performance characteristics.

Evidence:

```text
ASM02/evidence/task3_debug/perf_analysis.txt
ASM02/evidence/task3_debug/perf_report.txt
ASM02/evidence/task3_debug/perf.data
```

## 7.5 Task 4 - Real-Time Scheduling and Jitter Benchmark

### Objective

Compare timing behavior between the standard Linux scheduling policy and a real-time FIFO scheduling policy.

The benchmark is implemented in:

```text
ASM02/app/realtime_benchmark.c
```

### Benchmark configuration

| Parameter | Value |
|---|---|
| Samples | 1000 |
| Period | 1 ms |
| Scheduling modes | `SCHED_OTHER`, `SCHED_FIFO` |
| FIFO priority | 50 |
| Memory locking | `mlockall()` |
| Timing clock | Monotonic clock |

The benchmark requests memory locking and uses periodic timing to measure wake-up jitter.

### Recorded host benchmark results

| Metric | SCHED_OTHER | SCHED_FIFO (priority 50) |
|---|---:|---:|
| Minimum jitter | 54.42 µs | 1.42 µs |
| Average jitter | 73.59 µs | 34.86 µs |
| Maximum jitter | 134.56 µs | 1107.44 µs |
| P99 jitter | 109.35 µs | 203.75 µs |

### Histogram observations

| Jitter range | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Below 100 µs | 851 | 853 |
| 100-500 µs | 149 | 146 |
| 500-1000 µs | 0 | 0 |
| Above 1000 µs | 0 | 1 |

The average jitter in this recorded run decreased from 73.59 µs to 34.86 µs when using `SCHED_FIFO`, an improvement of approximately 52.6%.

However, the FIFO run also recorded a larger P99 jitter and a maximum outlier of 1107.44 µs. Therefore, this experiment does **not** show that `SCHED_FIFO` improves every latency metric in every run. It shows a lower average and minimum jitter in this particular run, alongside a worse tail latency and maximum outlier.

### Important measurement scope

These benchmark values were measured on the Ubuntu host. They are not measurements from physical ARM hardware and should not be presented as ARM/QEMU real-time performance results.

Evidence:

```text
ASM02/evidence/task4_realtime/realtime_analysis.txt
ASM02/evidence/task4_realtime/jitter_other.txt
ASM02/evidence/task4_realtime/jitter_fifo.txt
```

## 7.6 Build ASM02

### Build the sensor driver

```bash
cd ~/embedded_linux_course/ASM02/driver
make
```

Clean driver build outputs:

```bash
make clean
```

The driver Makefile uses the Linux 5.15 kernel tree and the ARM cross compiler.

### Build the user-space application

```bash
cd ~/embedded_linux_course/ASM02/app
make
```

The default target builds both host and ARM versions.

Build only the host version:

```bash
make host
```

Build only the ARM version:

```bash
make arm
```

Clean application binaries:

```bash
make clean
```

The Makefile uses:

- `gcc` for the host build.
- `arm-linux-gnueabihf-gcc` for the ARM build.
- `-Wall -Wextra -Werror -std=c11 -pthread`.
- Static linking for the ARM application.
- `-pthread` and `-lrt` for thread and POSIX real-time/message-queue support.

### Run the real-time benchmark

After building the benchmark executable:

```bash
cd ~/embedded_linux_course/ASM02/app
```

Run standard scheduling:

```bash
sudo ./realtime_benchmark other
```

Run FIFO scheduling:

```bash
sudo ./realtime_benchmark fifo
```

The FIFO test requires permission to use real-time scheduling. The benchmark also reports whether `mlockall()` succeeds.

## 7.7 ASM02 Test Evidence

Key evidence files:

| Area | Evidence file |
|---|---|
| Sensor driver on QEMU | `ASM02/evidence/task1_driver/sensor_driver_qemu_test.txt` |
| Circular buffer | `ASM02/evidence/task1_application/` |
| Processor and alert tests | `ASM02/evidence/task1_application/` |
| Sensor thread on QEMU | `ASM02/evidence/task1_application/` |
| POSIX Message Queues | `ASM02/evidence/task2_application/posix_mqueue_qemu_test.txt` |
| GDB analysis | `ASM02/evidence/task3_debug/gdb_analysis.txt` |
| Alert debugging | `ASM02/evidence/task3_debug/gdb_alert_debug.txt` |
| perf analysis | `ASM02/evidence/task3_debug/perf_analysis.txt` |
| perf report | `ASM02/evidence/task3_debug/perf_report.txt` |
| strace summary | `ASM02/evidence/task3_debug/strace_summary.txt` |
| Real-time benchmark | `ASM02/evidence/task4_realtime/realtime_analysis.txt` |

The evidence directories also contain supporting test output and analysis files for the individual assignment tasks.

---

# 8. TECHNICAL NOTES AND MEASUREMENT LIMITATIONS

To interpret the results correctly, keep the following distinctions in mind.

## 8.1 Host versus ARM target

Some tests run inside the ARM QEMU guest, while other measurements run natively on the Ubuntu x86_64 host.

Examples:

- ASM01 driver tests: ARM Linux test environment.
- ASM02 sensor driver integration test: QEMU ARM.
- ASM02 POSIX Message Queue test: QEMU ARM.
- ASM02 `perf` profiling results: host x86_64 application.
- ASM02 real-time jitter benchmark: host x86_64.
- ASM02 host sensor simulation: host-side application build.

A host measurement is useful for analyzing application behavior, but it is not equivalent to a measurement on physical ARM hardware.

## 8.2 SCHED_FIFO interpretation

Real-time scheduling can reduce average wake-up latency under some conditions, but it does not guarantee that every sample will have lower latency.

The recorded ASM02 benchmark demonstrates this distinction: average jitter improved in the FIFO run, while P99 and maximum jitter were worse due to a high-latency outlier.

A robust real-time evaluation should use repeated runs, report distributions and tail latency, document system load, and control the test environment.

## 8.3 Memory measurement interpretation

Values such as `VmSize`, `VmRSS`, and `VmData` describe different aspects of process memory.

They should not be treated as interchangeable. A very small RSS snapshot does not, by itself, prove that an application has negligible physical memory usage over its full execution.

## 8.4 Evidence and reproducibility

The evidence files are retained under each assignment's `evidence/` directory. Build outputs and temporary test artifacts may be generated locally and may not all be tracked by Git.

Before reproducing a test:

1. Check that the Linux 5.15 kernel source is available.
2. Check that the ARM cross compiler is installed.
3. Build the relevant module or application.
4. Boot the appropriate ARM QEMU image if the test requires the target environment.
5. Load the required kernel module.
6. Create or verify the device node.
7. Run the test and compare the output with the corresponding evidence file.

---

*End of Embedded Linux Course Documentation.*
