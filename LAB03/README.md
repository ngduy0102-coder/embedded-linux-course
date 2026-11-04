# LAB03 | Embedded Linux Systems
## Debugging Kernel & Performance Analysis

**Student:** Nguyễn Duy  
**Student ID:** SE192916  
**Target:** ARMv7-A Cortex-A9 / QEMU vexpress-a9  
**Kernel:** Linux 5.15.0

---

## 1. Objectives

- Remote debugging with GDB and gdbserver.
- Memory leak detection and analysis using Valgrind.
- System call tracing using strace.
- CPU performance profiling using perf.
- Real-time latency measurement using cyclictest.
- Comparison of standard Linux kernel and PREEMPT_RT.

## 2. Repository Structure

```text
LAB03/
├── README.md
├── apps/
│   ├── buggy_app.c
│   ├── leaky_app.c
│   ├── leaky_app_measure.c
│   └── perf_target.c
├── evidence/
│   ├── A1_qemu_gdbserver_runtime.txt
│   ├── A2_gdb_bug1_out_of_bounds.txt
│   ├── A2_gdb_bug2_use_after_free.txt
│   ├── A2_gdb_bug3_integer_overflow.txt
│   ├── B3_valgrind_memory_leaks.txt
│   ├── B4_valgrind_fixed.txt
│   ├── B5_arm_memory_measurement.txt
│   ├── C2_strace_analysis.txt
│   └── C3_C4_profiling_comparison.txt
├── scripts/
│   └── run_qemu.sh
└── output/
    ├── initramfs.cpio.gz
    ├── vexpress-v2p-ca9.dtb
    └── zImage
```

## 3. Lab Progress

| Section | Task | Status |
|---|---|---|
| A1 | QEMU and gdbserver runtime | Completed |
| A2 | GDB debugging: three bugs | Completed |
| B1 | Create memory leak application | Completed |
| B2 | Compile application for x86_64 host | Completed |
| B3 | Detect memory leaks using Valgrind | Completed |
| B4 | Fix memory leaks and verify | Completed |
| B5 | Measure ARM memory usage through `/proc` | Completed |
| C1 | Implement performance profiling application | Completed |
| C2 | System call analysis using strace | Completed |
| C3 | CPU profiling using perf | Completed |
| C4 | ARM cross-compilation and QEMU performance comparison | Partially completed |
| D1–D5 | Real-time latency analysis | Not started |
| Report | Final lab report | Pending |

## 4. Implementation and Evidence

### Part A – Remote Debugging

**A1 – QEMU and gdbserver runtime**
- Runtime evidence: `evidence/A1_qemu_gdbserver_runtime.txt`

**A2 – GDB debugging**
- Array out-of-bounds: `evidence/A2_gdb_bug1_out_of_bounds.txt`
- Use-after-free: `evidence/A2_gdb_bug2_use_after_free.txt`
- Integer overflow: `evidence/A2_gdb_bug3_integer_overflow.txt`

Source code: `apps/buggy_app.c`

### Part B – Memory Leak Analysis

- **B1:** Memory leak application: `apps/leaky_app.c`
- **B2:** Host x86_64 compilation with debug symbols.
- **B3:** Valgrind memory leak analysis: `evidence/B3_valgrind_memory_leaks.txt`
- **B4:** Fixed application and Valgrind verification: `evidence/B4_valgrind_fixed.txt`
- **B5:** ARM memory measurement: `evidence/B5_arm_memory_measurement.txt`

### Part C – Performance Profiling

- **C1:** Profiling application: `apps/perf_target.c`
- **C2:** System call analysis: `evidence/C2_strace_analysis.txt`
- **C3:** Host CPU profiling using perf.
- **C4:** ARM cross-compilation and QEMU execution-time comparison.

C3 and C4 results are documented in:

`evidence/C3_C4_profiling_comparison.txt`

**C4 limitation:** The ARM application was successfully executed in QEMU and its execution time was measured. However, direct ARM-side `strace` profiling was not completed because `strace` was unavailable in the guest environment.

### Part D – Real-Time Analysis

Planned tasks:

- D1: Understand cyclictest and latency metrics.
- D2: Measure latency using the standard kernel.
- D3: Generate latency histogram.
- D4: Analyze latency results.
- D5: Compare standard kernel and PREEMPT_RT theoretically.

Status: Not started.

## 5. Git Milestones

The work is committed progressively to preserve development history.

Recent milestones:

- `LAB03: establish debugging workspace baseline`
- GDB remote debugging and bug analysis
- Valgrind memory leak analysis
- ARM memory measurement
- Performance profiling and ARM comparison
- `LAB03: add strace system call analysis evidence`

---

**Note:** Evidence files contain the recorded results of the corresponding experiments. Generated binaries, temporary files, and build directories are not necessarily tracked in Git.
