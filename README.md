# EMBEDDED LINUX COURSE
## ASM01 & ASM02 — Linux Kernel Drivers and Embedded Applications

This document summarizes the implementation, architecture, build instructions, test procedures, and recorded results of ASM01 and ASM02.

The scope of this README is limited to **ASM01** and **ASM02**. LAB01, LAB02, and LAB03 documentation can be integrated into a repository-level README later.

---

# TABLE OF CONTENTS

- [1. Development Environment](#1-development-environment)
- [2. Repository Structure](#2-repository-structure)
- [3. ASM01 - IOCTL and Miscellaneous Device Drivers](#3-asm01---ioctl-and-miscellaneous-device-drivers)
  - [3.1 Overview](#31-overview)
  - [3.2 Task 1 - IOCTL Character Driver](#32-task-1---ioctl-character-driver)
  - [3.3 Task 2 - Miscellaneous Counter Driver](#33-task-2---miscellaneous-counter-driver)
  - [3.4 Build and Test ASM01](#34-build-and-test-asm01)
  - [3.5 ASM01 Test Results](#35-asm01-test-results)
- [4. ASM02 - Sensor Monitoring and Real-Time Application](#4-asm02---sensor-monitoring-and-real-time-application)
  - [4.1 Overview](#41-overview)
  - [4.2 Task 1 - Sensor Driver and Multithreaded Application](#42-task-1---sensor-driver-and-multithreaded-application)
  - [4.3 Task 2 - POSIX Message Queues and Memory Analysis](#43-task-2---posix-message-queues-and-memory-analysis)
  - [4.4 Task 3 - Debugging and Performance Analysis](#44-task-3---debugging-and-performance-analysis)
  - [4.5 Task 4 - Real-Time Scheduling and Jitter Benchmark](#45-task-4---real-time-scheduling-and-jitter-benchmark)
  - [4.6 Build ASM02](#46-build-asm02)
  - [4.7 ASM02 Test Evidence](#47-asm02-test-evidence)
- [5. Technical Notes and Measurement Limitations](#5-technical-notes-and-measurement-limitations)

---

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

Relevant source and evidence files:

```text
embedded_linux_course/
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
│       │   └── task1_test.log
│       └── task2/
│           └── task2_test.log
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
    │   └── test_*.c / test_*.sh
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

---

# 3. ASM01 - IOCTL AND MISCELLANEOUS DEVICE DRIVERS

## 3.1 Overview

ASM01 focuses on Linux kernel module development and communication between kernel space and user space.

The assignment contains two driver implementations:

| Task | Driver | Main concepts |
|---|---|---|
| Task 1 | IOCTL character device | Character device registration, file operations, IOCTL commands, user/kernel data transfer |
| Task 2 | Miscellaneous counter device | miscdevice, atomic operations, procfs, input validation |

Both drivers were built for the ARM target using the Linux kernel build system and tested in the QEMU environment.

## 3.2 Task 1 - IOCTL Character Driver

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

## 3.3 Task 2 - Miscellaneous Counter Driver

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

## 3.4 Build and Test ASM01

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

## 3.5 ASM01 Test Results

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

# 4. ASM02 - SENSOR MONITORING AND REAL-TIME APPLICATION

## 4.1 Overview

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

## 4.2 Task 1 - Sensor Driver and Multithreaded Application

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

## 4.3 Task 2 - POSIX Message Queues and Memory Analysis

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

## 4.4 Task 3 - Debugging and Performance Analysis

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

## 4.5 Task 4 - Real-Time Scheduling and Jitter Benchmark

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

## 4.6 Build ASM02

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

## 4.7 ASM02 Test Evidence

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

# 5. TECHNICAL NOTES AND MEASUREMENT LIMITATIONS

To interpret the results correctly, keep the following distinctions in mind.

## 5.1 Host versus ARM target

Some tests run inside the ARM QEMU guest, while other measurements run natively on the Ubuntu x86_64 host.

Examples:

- ASM01 driver tests: ARM Linux test environment.
- ASM02 sensor driver integration test: QEMU ARM.
- ASM02 POSIX Message Queue test: QEMU ARM.
- ASM02 `perf` profiling results: host x86_64 application.
- ASM02 real-time jitter benchmark: host x86_64.
- ASM02 host sensor simulation: host-side application build.

A host measurement is useful for analyzing application behavior, but it is not equivalent to a measurement on physical ARM hardware.

## 5.2 SCHED_FIFO interpretation

Real-time scheduling can reduce average wake-up latency under some conditions, but it does not guarantee that every sample will have lower latency.

The recorded ASM02 benchmark demonstrates this distinction: average jitter improved in the FIFO run, while P99 and maximum jitter were worse due to a high-latency outlier.

A robust real-time evaluation should use repeated runs, report distributions and tail latency, document system load, and control the test environment.

## 5.3 Memory measurement interpretation

Values such as `VmSize`, `VmRSS`, and `VmData` describe different aspects of process memory.

They should not be treated as interchangeable. A very small RSS snapshot does not, by itself, prove that an application has negligible physical memory usage over its full execution.

## 5.4 Evidence and reproducibility

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

# END OF ASM01 & ASM02 DOCUMENTATION
