#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/math64.h>

#include "sms_sensor.h"

#define DEVICE_NAME "sms_sensor"
#define CLASS_NAME  "sms_sensor_class"

static dev_t sms_dev;
static struct cdev sms_cdev;
static struct class *sms_class;
static struct device *sms_device;

static DEFINE_MUTEX(sms_mutex);

static u32 read_count;
static u32 interval_ms = SMS_DEFAULT_INTERVAL_MS;
static int last_temperature;
static int last_humidity;
static u64 start_time_ms;
static u64 last_read_ms;

static int sms_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int sms_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t sms_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    char buffer[128];
    int len;
    u32 random_value;
    int temperature;
    int humidity;
    u64 timestamp;

    if (count == 0)
        return 0;

    mutex_lock(&sms_mutex);

    timestamp = ktime_to_ms(ktime_get());

    if (last_read_ms && timestamp - last_read_ms < interval_ms) {
        mutex_unlock(&sms_mutex);
        return -EAGAIN;
    }

    random_value = get_random_u32();
    temperature = 1500 + (random_value % 3001);

    random_value = get_random_u32();
    humidity = 3000 + (random_value % 6001);

    len = scnprintf(buffer, sizeof(buffer), "%llu,%d.%02d,%d.%02d\n",
                    timestamp,
                    temperature / 100, temperature % 100,
                    humidity / 100, humidity % 100);

    if (count < len) {
        mutex_unlock(&sms_mutex);
        return -EINVAL;
    }

    if (copy_to_user(user_buf, buffer, len)) {
        mutex_unlock(&sms_mutex);
        return -EFAULT;
    }

    last_temperature = temperature;
    last_humidity = humidity;
    read_count++;
    last_read_ms = timestamp;

    mutex_unlock(&sms_mutex);

    return len;
}

static long sms_ioctl(struct file *file, unsigned int cmd,
                      unsigned long arg)
{
    u32 new_interval;

    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;

    if (copy_from_user(&new_interval, (u32 __user *)arg,
                       sizeof(new_interval)))
        return -EFAULT;

    if (new_interval < SMS_MIN_INTERVAL_MS)
        return -EINVAL;

    mutex_lock(&sms_mutex);
    interval_ms = new_interval;
    mutex_unlock(&sms_mutex);

    return 0;
}

static const struct file_operations sms_fops = {
    .owner          = THIS_MODULE,
    .open           = sms_open,
    .release        = sms_release,
    .read           = sms_read,
    .unlocked_ioctl = sms_ioctl,
    .llseek         = no_llseek,
};

static int sms_stats_show(struct seq_file *m, void *v)
{
    u64 uptime;

    mutex_lock(&sms_mutex);

    uptime = div_u64(ktime_to_ms(ktime_get()) - start_time_ms, 1000);

    seq_printf(m, "read_count: %u\n", read_count);
    seq_printf(m, "last_temperature: %d.%02d\n", last_temperature / 100, last_temperature % 100);
    seq_printf(m, "last_humidity: %d.%02d\n", last_humidity / 100, last_humidity % 100);
    seq_printf(m, "uptime: %llu seconds\n", uptime);
    seq_printf(m, "interval_ms: %u\n", interval_ms);

    mutex_unlock(&sms_mutex);

    return 0;
}

static int sms_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_stats_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init sms_init(void)
{
    int ret;

    start_time_ms = ktime_to_ms(ktime_get());

    ret = alloc_chrdev_region(&sms_dev, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&sms_cdev, &sms_fops);
    sms_cdev.owner = THIS_MODULE;

    ret = cdev_add(&sms_cdev, sms_dev, 1);
    if (ret < 0)
        goto err_unregister;

    sms_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sms_class)) {
        ret = PTR_ERR(sms_class);
        goto err_cdev;
    }

    sms_device = device_create(sms_class, NULL, sms_dev, NULL,
                               DEVICE_NAME);
    if (IS_ERR(sms_device)) {
        ret = PTR_ERR(sms_device);
        goto err_class;
    }

    if (!proc_create("sms_stats", 0444, NULL, &sms_proc_ops)) {
        ret = -ENOMEM;
        goto err_device;
    }

    pr_info("sms_sensor: initialized, major=%d\n", MAJOR(sms_dev));
    return 0;

err_device:
    device_destroy(sms_class, sms_dev);
err_class:
    class_destroy(sms_class);
err_cdev:
    cdev_del(&sms_cdev);
err_unregister:
    unregister_chrdev_region(sms_dev, 1);
    return ret;
}

static void __exit sms_exit(void)
{
    remove_proc_entry("sms_stats", NULL);
    device_destroy(sms_class, sms_dev);
    class_destroy(sms_class);
    cdev_del(&sms_cdev);
    unregister_chrdev_region(sms_dev, 1);

    pr_info("sms_sensor: unloaded\n");
}

module_init(sms_init);
module_exit(sms_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ASM02");
MODULE_DESCRIPTION("Sensor Monitoring System Character Driver");
MODULE_VERSION("1.0");
