# LAB-02 | Embedded Linux Systems
## Viết Device Driver & Quản lý File System

> **Kernel Module | Character Device | procfs/sysfs | MTD/JFFS2 | BusyBox Init**

**👤 Student:** Nguyễn Duy  
**🆔 MSSV:** SE192916  
**📚 Course:** Embedded Linux Systems  

---

## 📌 Thông tin bài lab

| Nội dung | Chi tiết |
|---|---|
| **Mã bài lab** | LAB-02 |
| **Tên bài lab** | Viết Device Driver & Quản lý File System |
| **Thời gian thực hiện** | 4 giờ (ở nhà 2 giờ - trên lớp 2 giờ) |
| **Chương liên quan** | Chapter 8, 9, 10, 11 |
| **Kiến trúc mục tiêu** | ARMv7-A – Cortex-A9 |
| **Môi trường mô phỏng** | QEMU `vexpress-a9` |
| **Điều kiện tiên quyết** | Hoàn thành LAB-01 |
| **Công cụ** | GCC cross-compiler, QEMU, nandsim, mtd-utils, BusyBox |
| **Hình thức nộp bài** | File nén + Báo cáo PDF |

---

# I. 🎯 Mục tiêu bài lab

Sau khi hoàn thành LAB-02, sinh viên có khả năng:

- Hiểu vòng đời của một **Linux Kernel Module**: init → register → hoạt động → exit.
- Tự viết một **Character Device Driver** với các phương thức:
  - `open`
  - `read`
  - `write`
  - `release`
- Xuất thông tin nội bộ của driver ra userspace thông qua:
  - `/proc`
  - `/sys` (**sysfs**)
- Sử dụng **MTD subsystem** để mô phỏng flash NAND.
- Phân vùng và thao tác với filesystem **JFFS2**.
- Tích hợp kernel module vào chuỗi khởi động **BusyBox init**.
- Tự động load module trong quá trình boot thông qua `rcS`.
- So sánh và lựa chọn filesystem phù hợp cho các hệ thống embedded.

---

# II. 📚 Kiến thức nền cần có

### 2.1. Device Driver Concepts

- Character / Block / Network driver.
- Major / Minor number.
- Vòng đời kernel module.
- `module_init()` / `module_exit()`.
- `register_chrdev()`.
- `struct file_operations`.
- `open()`, `read()`, `write()`, `release()`.
- `copy_to_user()`.
- `copy_from_user()`.
- `insmod`, `rmmod`, `lsmod`, `modinfo`, `dmesg`.

### 2.2. procfs & sysfs

- `/proc` dùng để cung cấp thông tin trạng thái hệ thống / driver.
- `/sys` dùng để expose các thuộc tính của kernel objects và devices.
- `DEVICE_ATTR()`.
- `sysfs_create_group()`.
- `proc_create()`.
- `seq_file`.

### 2.3. Flash / MTD / JFFS2

- Raw NAND flash.
- MTD subsystem.
- `nandsim`.
- MTD partition.
- `mtd-utils`.
- Erase / write behavior của NAND.
- JFFS2 filesystem.
- Persistence sau khi mount / unmount.

### 2.4. BusyBox Init

- `/etc/inittab`.
- `/etc/init.d/rcS`.
- `sysinit`.
- `insmod`.
- Boot sequence của embedded Linux.

---

# III. 📁 Cấu trúc thư mục

```text
LAB02/
├── README.md
├── configs/
├── driver/
├── evidence/
│   ├── 01_workspace.txt
│   ├── 02_prepare_artifacts.txt
│   └── ...
├── output/
│   ├── zImage
│   ├── vexpress-v2p-ca9.dtb
│   └── initramfs.cpio.gz
└── rootfs/
