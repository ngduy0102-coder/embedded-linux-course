Student: Nguyen Duy
Student ID: SE192916
Platform: ARMv7-A Cortex-A9 / QEMU vexpress-a9
Kernel: Linux 5.15.0
Toolchain: arm-linux-gnueabihf-gcc-11
LAB-02 builds on the working system from LAB-01 and focuses on:
- Character Device Driver
- Kernel Module
- /proc and /sys
- MTD / NAND simulation
- JFFS2 filesystem
- BusyBox init / rcS
- Automatic kernel-module loading during boot
- Driver testing and technical analysis
```
LAB02/
├── configs/       # Configuration files
├── driver/        # Kernel module source and Makefile
├── evidence/      # Logs and verification evidence
├── output/        # LAB-02 generated artifacts
└── rootfs/        # Root filesystem / initramfs
```
- Create LAB02 directory structure
- Create configs/
- Create driver/
- Create evidence/
- Create output/
- Create rootfs/
- Save workspace evidence
- Commit and push to GitHub
Commit: 624e136
- Verify zImage
- Verify vexpress-v2p-ca9.dtb
- Verify initramfs.cpio.gz
- Prepare LAB-02 output directory
- Verify required tools
- Save preparation evidence
- Commit and push
Implement a character device driver named lab2_driver and communicate through:
```
/dev/lab2
```
with:
```
Major number: 240
Minor number: 0
```
- Create lab2_driver.c
- Implement module initialization
- Implement module exit
- Register character device
- Define file_operations
- Implement open
- Implement read
- Implement write
- Implement release
- Use copy_to_user()
- Use copy_from_user()
- Protect shared data with mutex
- Build lab2_driver.ko
- Check with modinfo
- Load with insmod
- Check with lsmod
- Create /dev/lab2
- Test write/read
- Check kernel messages with dmesg
- Module build log
- modinfo output
- insmod / lsmod output
- /dev/lab2 test
- dmesg output
Expose driver information through:
```
/proc/lab2_info
```
and device attributes through:
```
/sys/class/lab2_class/lab2/
```
- Implement /proc/lab2_info
- Use seq_file
- Implement single_open()
- Show driver statistics
- Test before write
- Test after write
- Create sysfs class/device
- Define device attributes
- Implement read attributes
- Implement writable attributes where required
- Use DEVICE_ATTR_RO / appropriate attribute macros
- Verify files under /sys/class/lab2_class/lab2/
- Test all attributes
- /proc/lab2_info before write
- /proc/lab2_info after write
- Complete sysfs attribute output
- Kernel log
Use nandsim to simulate NAND flash and work with the Linux MTD subsystem and JFFS2 filesystem.
- Check/install mtd-utils
- Load nandsim
- Verify /dev/mtd*
- Check MTD information with mtdinfo
- Erase NAND with flash_erase
- Prepare JFFS2 data
- Create JFFS2 image
- Write image with nandwrite
- Mount JFFS2
- Read stored files
- Write runtime data
- Unmount filesystem
- Mount again
- Verify persistence
- Dump raw MTD data with nanddump
- Inspect raw data
- Remove nandsim
- mtdinfo
- flash_erase
- nandwrite
- JFFS2 mount
- Read/write test
- Persistence test
- nanddump
- Raw data inspection
- Cleanup
Update the root filesystem so that:
```
BusyBox init
    ↓
/etc/inittab
    ↓
/etc/init.d/rcS
    ↓
insmod lab2_driver.ko
    ↓
/dev/lab2
```
The module must be loaded automatically during QEMU boot.
- Update /etc/init.d/rcS
- Mount /proc
- Mount /sys
- Mount /tmp
- Prepare /dev
- Set hostname
- Add automatic insmod
- Create /dev/lab2
- Update /etc/inittab
- Build updated initramfs
- Boot QEMU
- Verify automatic module loading
- Verify /dev/lab2
- Run driver test
- Run procfs/sysfs test
- rcS
- inittab
- QEMU boot log
- Auto-load module output
- /dev/lab2
- Driver test output
- procfs/sysfs test output
The official LAB-02 requires five technical analysis questions:
- Q1 — copy_to_user() / copy_from_user() vs memcpy()
  - Kernel space vs user space
  - User/kernel virtual addresses
  - Why direct memcpy() is inappropriate
- Q2 — Mutex and race condition
  - Why lab2_mutex is required
  - Concurrent write()
  - Race condition and possible consequences
- Q3 — procfs vs sysfs
  - Differences
  - Appropriate information for /proc
  - Appropriate device attributes for /sys
- Q4 — NAND erase and JFFS2
  - NAND flash erase characteristics
  - Difference from ext4/block storage
  - JFFS2 log-structured design
- Q5 — BusyBox rcS error handling
  - What happens when insmod fails
  - Critical vs non-critical errors
  - Improving rcS error handling
- Student information
- Environment information
- Driver architecture diagram
- User space → system call → VFS → file_operations
- Data flow with copy_from_user()
- /proc and /sys structure
- Part A logs
- Part B logs
- Part C logs
- Part D QEMU boot logs
- Filesystem comparison table
- Analysis questions
- At least two difficulties/errors and solutions
- Final review
This LAB follows the official:

LAB-02 | Embedded Linux Systems | Viết Device Driver & Quản lý File System
