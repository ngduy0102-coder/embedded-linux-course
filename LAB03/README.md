# LAB03 | Embedded Linux Systems
## Debugging Kernel & Phân tích Hiệu năng

**Student:** Nguyễn Duy  
**MSSV:** SE192916  
**Target:** ARMv7-A Cortex-A9 / QEMU vexpress-a9  
**Kernel:** Linux 5.15.0

---

## 1. Objectives

- GDB Remote Debugging
- Valgrind memory analysis
- strace system-call analysis
- perf performance analysis
- cyclictest latency measurement
- Compare standard kernel and PREEMPT_RT latency

---

## 2. Workspace

```text
LAB03/
├── README.md
├── apps/
├── scripts/
├── evidence/
├── logs/
├── output/
├── rt_kernel/
└── bao_cao/
```

---

## 3. Progress

| Part | Task | Status |
|---|---|---|
| A | ARM gdbserver + QEMU runtime | IN PROGRESS |
| A | GDB remote debugging | TODO |
| B | Valgrind | TODO |
| C | strace | TODO |
| C | perf | TODO |
| D | cyclictest | TODO |
| D | PREEMPT_RT comparison | TODO |
| Report | Final report | TODO |

---

## 4. Git Milestones

The LAB03 work is committed progressively so that the development process can be reviewed.

- `LAB03: establish debugging workspace baseline`
- ARM gdbserver + QEMU runtime
- GDB remote debugging
- Valgrind analysis
- strace/perf analysis
- cyclictest latency analysis
- final documentation
