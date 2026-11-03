# LAB-02 | Embedded Linux Systems
## Viết Device Driver & Quản lý File System

> **Kernel Module | Character Device | procfs/sysfs | MTD/JFFS2 | BusyBox Init**

**Student:** Nguyễn Duy  
**MSSV:** SE192916  
**Course:** Embedded Linux Systems  

---

## 1. Thông tin bài lab

| Nội dung | Chi tiết |
|---|---|
| Mã bài lab | LAB-02 |
| Tên bài lab | Viết Device Driver & Quản lý File System |
| Thời gian thực hiện | 4 giờ |
| Chương liên quan | Chapter 8, 9, 10, 11 |
| Kiến trúc mục tiêu | ARMv7-A – Cortex-A9 |
| Môi trường mô phỏng | QEMU `vexpress-a9` |
| Kernel | Linux 5.15.0 |
| Cross compiler | `arm-linux-gnueabihf-gcc-11` |
| Công cụ | GCC cross-compiler, QEMU, nandsim, mtd-utils, BusyBox |

---

# I. Mục tiêu

LAB-02 thực hiện:

- Xây dựng Linux Character Device Driver.
- Đăng ký `/dev/lab2` với major `240`.
- Implement `open()`, `read()`, `write()`, `release()`.
- Sử dụng `copy_to_user()` và `copy_from_user()`.
- Bảo vệ dữ liệu bằng mutex.
- Xuất trạng thái qua `/proc/lab2_info`.
- Expose sysfs attributes:
  - `buffer_len`
  - `open_count`
  - `last_data`
- Mô phỏng NAND bằng `nandsim`.
- Tạo và kiểm tra JFFS2.
- Tích hợp driver vào BusyBox init/`rcS`.
- Tự động load driver khi boot.

---

# II. Kiến thức nền

## 2.1. Character Device

- Major / Minor number.
- Kernel Module.
- `module_init()` / `module_exit()`.
- `register_chrdev()`.
- `struct file_operations`.
- `open()`, `read()`, `write()`, `release()`.
- `copy_to_user()`.
- `copy_from_user()`.

## 2.2. procfs / sysfs

- `/proc` cung cấp thông tin trạng thái.
- `/sys` expose thuộc tính của kernel objects/devices.
- `proc_create()`.
- `single_open()`.
- `seq_file`.
- `DEVICE_ATTR_RO()`.

## 2.3. MTD / NAND / JFFS2

- Raw NAND flash.
- MTD subsystem.
- `nandsim`.
- `mtd-utils`.
- Erase / write behavior.
- JFFS2.
- Persistence sau mount/unmount.

## 2.4. BusyBox Init

- `/etc/inittab`.
- `/etc/init.d/rcS`.
- `sysinit`.
- `insmod`.
- Boot sequence.

---

# III. Cấu trúc thư mục

```text
LAB02/
├── README.md
├── configs/
├── driver/
│   ├── lab2_driver.c
│   ├── Makefile
│   └── nandsim/
├── evidence/
│   ├── 01_workspace.txt
│   ├── 02_prepare_artifacts.txt
│   ├── ...
│   └── 19_d5_full_check.txt
├── jffs2_data/
│   └── test.txt
├── jffs2_host_data/
│   └── test.txt
├── output/
│   ├── zImage
│   ├── vexpress-v2p-ca9.dtb
│   ├── initramfs.cpio.gz
│   ├── jffs2_host_test.img
│   ├── jffs2_test.img
│   └── nand_host_dump.bin
└── rootfs/
    ├── etc/
    │   ├── inittab
    │   └── init.d/
    │       └── rcS
    ├── lib/modules/5.15.0/
    │   ├── lab2_driver.ko
    │   └── nandsim.ko
    └── usr/bin/
        ├── test_driver.sh
        └── test_procfs.sh
```

Build artifact trung gian như `.o`, `.cmd`, `Module.symvers`, `modules.order` không được Git track.

---

# IV. Part A — Character Device Driver

## 4.1. Thông số

```text
Device       : /dev/lab2
Major        : 240
Minor        : 0
Buffer size  : 1024 bytes
```

Source:

```text
driver/lab2_driver.c
driver/Makefile
```

Driver implement:

```text
open()
read()
write()
release()
```

## 4.2. Kernel/User space

Driver sử dụng:

```text
copy_from_user()
copy_to_user()
```

để truyền dữ liệu giữa userspace và kernel space.

Không dùng `memcpy()` trực tiếp giữa user-space buffer và kernel-space buffer.

## 4.3. Mutex

Driver sử dụng mutex để bảo vệ buffer và state khi có nhiều thao tác truy cập đồng thời.

Nếu không có đồng bộ, nhiều tiến trình có thể cùng truy cập buffer và gây race condition.

## 4.4. Runtime test

Script:

```text
rootfs/usr/bin/test_driver.sh
```

Kiểm tra:

- module;
- `/dev/lab2`;
- write;
- read;
- multiple write/read;
- kernel messages.

Kết quả:

```text
=== Test PASSED ===
```

---

# V. Part B — procfs và sysfs

## 5.1. procfs

Driver tạo:

```text
/proc/lab2_info
```

Thông tin:

```text
LAB-02 Character Device Driver
Device: /dev/lab2
Major: 240
Open count: N
Buffer length: N
```

## 5.2. sysfs

Class:

```text
/sys/class/lab2_class/lab2/
```

Attributes:

```text
buffer_len
open_count
last_data
```

Runtime:

```text
buffer_len : 30
open_count : 9
last_data : Embedded Linux Lab2 Test Data
```

Script:

```text
rootfs/usr/bin/test_procfs.sh
```

Kết quả:

```text
=== procfs/sysfs Test PASSED ===
```

---

# VI. Part C — MTD / NAND / JFFS2

> C2–C5 chính thức được thực hiện trên HOST Ubuntu theo yêu cầu của LAB-02.

## 6.1. C2 — NAND simulator

Load:

```bash
sudo modprobe nandsim
```

Kiểm tra:

```bash
cat /proc/mtd
```

Host tạo NAND simulator tại:

```text
/dev/mtd1
```

`/dev/mtd0` đã được sử dụng bởi MTD device khác trên host.

## 6.2. Tạo JFFS2 image

Test data:

```text
jffs2_host_data/test.txt
```

Nội dung:

```text
LAB02 JFFS2 HOST persistence test - SE192916
```

Tạo image:

```bash
mkfs.jffs2 \
  --eraseblock=0x4000 \
  --no-cleanmarkers \
  --output=output/jffs2_host_test.img \
  --root=jffs2_host_data
```

## 6.3. Erase NAND

```bash
sudo flash_erase /dev/mtd1 0 0
```

Kết quả:

```text
Erasing 131072 Kibyte @ 0 -- 100 % complete
```

## 6.4. Write JFFS2

```bash
sudo nandwrite -p /dev/mtd1 output/jffs2_host_test.img
```

Kết quả:

```text
Writing data to block 0 at offset 0x0
```

## 6.5. Mount

```bash
sudo modprobe mtdblock
```

Device:

```text
/dev/mtdblock1
```

Mount point:

```text
/mnt/lab2_jffs2
```

File test được đọc thành công sau mount.

## 6.6. Persistence

Runtime data:

```text
JFFS2 HOST runtime persistence - SE192916
```

Sau:

```text
unmount
    ↓
mount lại
    ↓
đọc file
```

file vẫn tồn tại trong cùng NAND simulator session.

## 6.7. NAND dump

```bash
sudo nanddump -o -l 512 /dev/mtd1 -f output/nand_host_dump.bin
```

JFFS2 magic:

```text
85 19
```

được xác nhận tại offset 0.

## 6.8. Cleanup

```bash
sudo umount /mnt/lab2_jffs2
sudo modprobe -r nandsim
```

Sau cleanup:

- `nandsim` được remove.
- `/dev/mtd1` không còn.
- `/dev/mtdblock1` không còn.

Evidence:

```text
evidence/18_c2_c5_host_nand_jffs2.txt
```

---

# VII. Part D — BusyBox Init và Boot

Rootfs chứa:

```text
rootfs/etc/inittab
rootfs/etc/init.d/rcS
rootfs/lib/modules/5.15.0/lab2_driver.ko
rootfs/lib/modules/5.15.0/nandsim.ko
rootfs/usr/bin/test_driver.sh
rootfs/usr/bin/test_procfs.sh
```

`rcS` thực hiện:

```text
mount proc
mount sysfs
mount tmpfs
mount devtmpfs
load lab2_driver.ko
create /dev/lab2
```

Driver được tự động load khi boot.

Boot artifacts:

```text
output/zImage
output/vexpress-v2p-ca9.dtb
output/initramfs.cpio.gz
```

Boot log xác nhận:

```text
[ LAB-02 ] Starting system initialization...
[ LAB-02 ] Loading lab2_driver module...
lab2: driver loaded, major=240
[ LAB-02 ] lab2_driver loaded OK
[ LAB-02 ] /dev/lab2 created (major=240, minor=0)
[ LAB-02 ] System initialization complete!
```

---

# VIII. D5 — Full Runtime Verification

D5 gồm 7 kiểm tra:

```text
[1] lsmod
[2] /dev/lab2
[3] test_driver.sh
[4] test_procfs.sh
[5] /proc/lab2_info
[6] sysfs attributes
[7] dmesg
```

## 8.1. Module

```text
lab2_driver 16384 0 - Live 0x7f000000 (O)
```

## 8.2. Device node

```text
crw-rw-rw-    1 root root 240, 0 /dev/lab2
```

## 8.3. Character driver

```text
=== Test PASSED ===
```

Read-back:

```text
Hello from userspace, LAB-02!
```

Multiple messages:

```text
Message_1
Message_2
Message_3
```

## 8.4. procfs/sysfs

```text
Open count: 9
Buffer length: 30

buffer_len : 30
open_count : 9
last_data : Embedded Linux Lab2 Test Data
```

## 8.5. Final status

```text
D5 FULL RUNTIME CHECK: PASS
```

Full evidence:

```text
evidence/19_d5_full_check.txt
```

---

# IX. Evidence

Evidence được lưu trong:

```text
evidence/
```

Các checkpoint:

| Evidence | Nội dung |
|---|---|
| 01 | Workspace |
| 02 | Prepare artifacts |
| 03 | Driver build |
| 04–08 | Driver runtime / read-write / procfs / sysfs |
| 09–13 | NAND simulator và JFFS2 workflow |
| 14–17 | `last_data` và procfs/sysfs |
| 18 | Host NAND/JFFS2 C2–C5 |
| 19 | D5 full runtime check |

Evidence quan trọng:

```text
evidence/18_c2_c5_host_nand_jffs2.txt
evidence/19_d5_full_check.txt
```

---

# X. Output Artifacts

```text
output/
├── zImage
├── vexpress-v2p-ca9.dtb
├── initramfs.cpio.gz
├── jffs2_host_test.img
├── jffs2_test.img
└── nand_host_dump.bin
```

| File | Mục đích |
|---|---|
| `zImage` | Linux kernel ARM |
| `vexpress-v2p-ca9.dtb` | Device Tree cho QEMU |
| `initramfs.cpio.gz` | Root filesystem |
| `jffs2_host_test.img` | JFFS2 image cho host workflow |
| `jffs2_test.img` | JFFS2 test image |
| `nand_host_dump.bin` | NAND dump sau khi ghi JFFS2 |

---

# XI. Git Repository

Build artifact trung gian không cần thiết không được Git track:

```text
*.o
*.cmd
Module.symvers
modules.order
```

Các source/artifact chính:

```text
driver/lab2_driver.c
driver/Makefile
driver/nandsim/nandsim.ko
evidence/
output/
rootfs/
```

Git checkpoint cuối:

```text
e133689 LAB02: verify full D5 runtime checks
```

---

# XII. Kết quả tổng hợp

| Hạng mục | Trạng thái |
|---|---|
| Character Device Driver | PASS |
| `open/read/write/release` | PASS |
| procfs | PASS |
| sysfs | PASS |
| `last_data` | PASS |
| NAND simulator | PASS |
| JFFS2 host workflow | PASS |
| NAND dump | PASS |
| BusyBox module autoload | PASS |
| D5 runtime verification | PASS |

**LAB-02 RUNTIME IMPLEMENTATION: PASS**

