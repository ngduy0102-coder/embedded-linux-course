#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "lab2"
#define CLASS_NAME  "lab2_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 240

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
static int open_count;

static struct class *lab2_class;
static struct device *lab2_device;
static DEFINE_MUTEX(lab2_mutex);
static struct proc_dir_entry *proc_lab2_info;

/* =========================
 * Character device
 * ========================= */

static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);
    open_count++;
    pr_info("lab2: device opened (count=%d)\n", open_count);
    mutex_unlock(&lab2_mutex);

    return 0;
}

static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2: device closed\n");
    return 0;
}

static ssize_t lab2_read(struct file *file,
                         char __user *user_buffer,
                         size_t count,
                         loff_t *offset)
{
    size_t bytes_to_copy;

    mutex_lock(&lab2_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }

    bytes_to_copy = min(count,
                        buffer_len - (size_t)*offset);

    if (copy_to_user(user_buffer,
                     device_buffer + *offset,
                     bytes_to_copy)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    *offset += bytes_to_copy;

    mutex_unlock(&lab2_mutex);

    pr_info("lab2: read %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}

static ssize_t lab2_write(struct file *file,
                          const char __user *user_buffer,
                          size_t count,
                          loff_t *offset)
{
    size_t bytes_to_copy;

    bytes_to_copy = min(count,
                        (size_t)(BUFFER_SIZE - 1));

    mutex_lock(&lab2_mutex);

    if (copy_from_user(device_buffer,
                       user_buffer,
                       bytes_to_copy)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    device_buffer[bytes_to_copy] = '\0';
    buffer_len = bytes_to_copy;

    mutex_unlock(&lab2_mutex);

    pr_info("lab2: wrote %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}

static const struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .read    = lab2_read,
    .write   = lab2_write,
    .release = lab2_release,
};


/* =========================
 * procfs: /proc/lab2_info
 * ========================= */

static int lab2_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&lab2_mutex);

    seq_printf(m, "LAB-02 Character Device Driver\n");
    seq_printf(m, "Device: /dev/%s\n", DEVICE_NAME);
    seq_printf(m, "Major: %d\n", DRIVER_MAJOR);
    seq_printf(m, "Open count: %d\n", open_count);
    seq_printf(m, "Buffer length: %zu\n", buffer_len);

    mutex_unlock(&lab2_mutex);

    return 0;
}

static int lab2_proc_open(struct inode *inode,
                          struct file *file)
{
    return single_open(file, lab2_proc_show, NULL);
}

static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};


/* =========================
 * sysfs attributes
 * ========================= */

static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);
    ret = sysfs_emit(buf, "%d\n", open_count);
    mutex_unlock(&lab2_mutex);

    return ret;
}

static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);
    ret = sysfs_emit(buf, "%zu\n", buffer_len);
    mutex_unlock(&lab2_mutex);

    return ret;
}

static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(buffer_len);


/* =========================
 * module init
 * ========================= */

static int __init lab2_init(void)
{
    int ret;

    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &lab2_fops);

    if (ret < 0) {
        pr_err("lab2: failed to register major %d\n",
               major_number);
        return ret;
    }

    lab2_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }

    lab2_device = device_create(lab2_class,
                                NULL,
                                MKDEV(major_number, 0),
                                NULL,
                                DEVICE_NAME);

    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }

    proc_lab2_info = proc_create("lab2_info",
                                 0444,
                                 NULL,
                                 &lab2_proc_fops);

    if (!proc_lab2_info) {
        device_destroy(lab2_class,
                       MKDEV(major_number, 0));
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENOMEM;
    }

    ret = device_create_file(lab2_device,
                             &dev_attr_open_count);

    if (ret) {
        proc_remove(proc_lab2_info);
        device_destroy(lab2_class,
                       MKDEV(major_number, 0));
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    ret = device_create_file(lab2_device,
                             &dev_attr_buffer_len);

    if (ret) {
        device_remove_file(lab2_device,
                           &dev_attr_open_count);
        proc_remove(proc_lab2_info);
        device_destroy(lab2_class,
                       MKDEV(major_number, 0));
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    buffer_len = 0;
    open_count = 0;

    pr_info("lab2: driver loaded, major=%d\n",
            major_number);

    return 0;
}


/* =========================
 * module exit
 * ========================= */

static void __exit lab2_exit(void)
{
    device_remove_file(lab2_device,
                       &dev_attr_buffer_len);

    device_remove_file(lab2_device,
                       &dev_attr_open_count);

    proc_remove(proc_lab2_info);

    device_destroy(lab2_class,
                   MKDEV(major_number, 0));

    class_destroy(lab2_class);

    unregister_chrdev(major_number,
                      DEVICE_NAME);

    pr_info("lab2: driver unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Duy - SE192916");
MODULE_DESCRIPTION("LAB-02 Character Device Driver");
MODULE_VERSION("1.0");
