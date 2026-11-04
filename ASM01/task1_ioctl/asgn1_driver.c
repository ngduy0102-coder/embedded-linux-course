#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>

#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Duy - SE192916");
MODULE_DESCRIPTION("ASM1: Character Device Driver with IOCTL");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;

static int open_count = 0;
static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;

/* 0: Kernel mode, 1: Echo mode */
static int driver_mode = 1;

static struct class *asgn1_class = NULL;
static struct device *asgn1_device = NULL;

static DEFINE_MUTEX(asgn1_mutex);


/* =========================
 * open()
 * ========================= */
static int asgn1_open(struct inode *inode, struct file *file)
{
    mutex_lock(&asgn1_mutex);

    open_count++;

    pr_info("asgn1_driver: device opened (count=%d)\n",
            open_count);

    mutex_unlock(&asgn1_mutex);

    return 0;
}


/* =========================
 * release()
 * ========================= */
static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1_driver: device closed\n");

    return 0;
}


/* =========================
 * read()
 * ========================= */
static ssize_t asgn1_read(struct file *file,
                         char __user *user_buffer,
                         size_t count,
                         loff_t *offset)
{
    int bytes_to_copy;

    mutex_lock(&asgn1_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    bytes_to_copy = min(count,
                        (size_t)(buffer_len - *offset));

    if (copy_to_user(user_buffer,
                     device_buffer + *offset,
                     bytes_to_copy)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    *offset += bytes_to_copy;
    read_count++;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: sent %d bytes to user\n",
            bytes_to_copy);

    return bytes_to_copy;
}


/* =========================
 * write()
 * ========================= */
static ssize_t asgn1_write(struct file *file,
                          const char __user *user_buffer,
                          size_t count,
                          loff_t *offset)
{
    int bytes_to_copy;
    int prefix_len = 0;

    mutex_lock(&asgn1_mutex);

    if (driver_mode == 0)
        prefix_len = 8;   /* length of "[KERNEL]" */

    if (count > (size_t)(BUFFER_SIZE - 1 - prefix_len))
        bytes_to_copy = BUFFER_SIZE - 1 - prefix_len;
    else
        bytes_to_copy = count;

    memset(device_buffer, 0, BUFFER_SIZE);

    if (driver_mode == 0)
        strscpy(device_buffer, "[KERNEL]", BUFFER_SIZE);

    if (copy_from_user(device_buffer + prefix_len,
                       user_buffer,
                       bytes_to_copy)) {

        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    device_buffer[prefix_len + bytes_to_copy] = '\0';

    buffer_len = prefix_len + bytes_to_copy;
    write_count++;
    last_write_size = bytes_to_copy;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: received %d bytes: [%s]\n",
            bytes_to_copy,
            device_buffer);

    return bytes_to_copy;
}

static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    pr_info("asgn1_driver: ioctl cmd=0x%x\n", cmd);

    switch (cmd) {
        case ASGN1_RESET_BUFFER:
            mutex_lock(&asgn1_mutex);

            memset(device_buffer, 0, BUFFER_SIZE);
            buffer_len = 0;
            write_count = 0;
            read_count = 0;

            mutex_unlock(&asgn1_mutex);

            pr_info("asgn1_driver: ioctl RESET_BUFFER\n");

            return 0;

        case ASGN1_GET_STATS:
        {
            struct asgn1_stats stats;

            mutex_lock(&asgn1_mutex);

            stats.open_count = open_count;
            stats.write_count = write_count;
            stats.read_count = read_count;
            stats.buffer_len = buffer_len;
            stats.last_write_size = last_write_size;

            mutex_unlock(&asgn1_mutex);

            if (copy_to_user((struct asgn1_stats __user *)arg,
                             &stats,
                             sizeof(stats)))
                return -EFAULT;

            pr_info("asgn1_driver: ioctl GET_STATS\n");

            return 0;
        }

        case ASGN1_SET_MODE:
        {
            int mode;

            if (copy_from_user(&mode,
                               (int __user *)arg,
                               sizeof(mode)))
                return -EFAULT;

            if (mode != 0 && mode != 1)
                return -EINVAL;

            mutex_lock(&asgn1_mutex);
            driver_mode = mode;
            mutex_unlock(&asgn1_mutex);

            pr_info("asgn1_driver: ioctl SET_MODE mode=%d\n", mode);

            return 0;
        }

        case ASGN1_GET_VERSION:
        {
            char version[64] = "asgn1_driver v1.0 - SE192916";

            if (copy_to_user((char __user *)arg,
                             version,
                             sizeof(version)))
                return -EFAULT;

            pr_info("asgn1_driver: ioctl GET_VERSION\n");

            return 0;
        }

        default:
            return -ENOTTY;
    }
}


/* =========================
 * FILE OPERATIONS
 * ========================= */
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = asgn1_open,
    .read = asgn1_read,
    .write = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,
    .release = asgn1_release,
};


/* =========================
 * MODULE INIT
 * ========================= */
static int __init asgn1_init(void)
{
    int ret;

    pr_info("asgn1_driver: initializing...\n");


    /* Register character device */
    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &fops);

    if (ret < 0) {
        pr_err("asgn1_driver: failed to register major number %d\n",
               major_number);

        return ret;
    }


    /* Create class */
    asgn1_class = class_create(THIS_MODULE,
                              CLASS_NAME);

    if (IS_ERR(asgn1_class)) {
        unregister_chrdev(major_number,
                         DEVICE_NAME);

        pr_err("asgn1_driver: failed to create class\n");

        return PTR_ERR(asgn1_class);
    }


    /* Create device */
    asgn1_device = device_create(asgn1_class,
                                NULL,
                                MKDEV(major_number, 0),
                                NULL,
                                DEVICE_NAME);

    if (IS_ERR(asgn1_device)) {
        class_destroy(asgn1_class);

        unregister_chrdev(major_number,
                         DEVICE_NAME);

        pr_err("asgn1_driver: failed to create device\n");

        return PTR_ERR(asgn1_device);
    }

    return 0;

}


/* =========================
 * MODULE EXIT
 * ========================= */
static void __exit asgn1_exit(void)
{
    /* Remove device */
    device_destroy(asgn1_class,
                   MKDEV(major_number, 0));


    /* Remove class */
    class_destroy(asgn1_class);


    /* Unregister character device */
    unregister_chrdev(major_number,
                     DEVICE_NAME);


    pr_info("asgn1_driver: unloaded\n");
}


module_init(asgn1_init);
module_exit(asgn1_exit);
