#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Duy - SE192916");
MODULE_DESCRIPTION("ASM1 Task 2: Misc Counter Device Driver");
MODULE_VERSION("1.0");

/* Main counter and statistics */
static atomic_t counter = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);

/* /proc entry */
static struct proc_dir_entry *proc_entry;

/*
 * Read the current counter value and increment it after a successful read.
 *
 * @file: file pointer
 * @user_buffer: user-space destination buffer
 * @count: number of bytes requested by user
 * @ppos: current file position
 *
 * Returns: number of bytes copied, 0 at EOF, or a negative error code.
 */
static ssize_t counter_read(struct file *file,
                            char __user *user_buffer,
                            size_t count,
                            loff_t *ppos)
{
    char buffer[32];
    int len;
    int value;

    if (*ppos > 0)
        return 0;

    value = atomic_read(&counter);

    len = snprintf(buffer, sizeof(buffer), "%d\n", value);

    if (count < len)
        return -EINVAL;

    if (copy_to_user(user_buffer, buffer, len))
        return -EFAULT;

    *ppos = len;

    atomic_inc(&counter);
    atomic_inc(&total_reads);

    pr_info("counter_driver: read value=%d\n", value);

    return len;
}

/*
 * Set the counter from an ASCII integer provided by user space.
 *
 * @file: file pointer
 * @user_buffer: user-space source buffer
 * @count: number of bytes written
 * @ppos: current file position
 *
 * Returns: number of bytes consumed or a negative error code.
 */
static ssize_t counter_write(struct file *file,
                             const char __user *user_buffer,
                             size_t count,
                             loff_t *ppos)
{
    char buffer[32];
    int value;
    int ret;

    if (count == 0 || count >= sizeof(buffer))
        return -EINVAL;

    if (copy_from_user(buffer, user_buffer, count))
        return -EFAULT;

    buffer[count] = '\0';

    ret = kstrtoint(buffer, 10, &value);
    if (ret)
        return -EINVAL;

    if (value < 0)
        return -EINVAL;

    atomic_set(&counter, value);
    atomic_inc(&total_writes);

    pr_info("counter_driver: counter set to %d\n", value);

    return count;
}

/*
 * Display counter statistics through /proc/counter_info.
 *
 * @m: seq_file used to generate the output
 * @v: unused parameter
 *
 * Returns: 0 on success.
 */
static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Current counter: %d\n", atomic_read(&counter));
    seq_printf(m, "Total reads/increments: %d\n",
               atomic_read(&total_reads));
    seq_printf(m, "Total writes/resets: %d\n",
               atomic_read(&total_writes));

    return 0;
}

/*
 * Open /proc/counter_info.
 *
 * @inode: proc inode
 * @file: file pointer
 *
 * Returns: result of single_open().
 */
static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_fops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* File operations */
static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read = counter_read,
    .write = counter_write,
};

/* Misc device */
static struct miscdevice counter_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "counter",
    .fops = &counter_fops,
    .mode = 0666,
};


static int __init counter_init(void)
{
    int ret;

    atomic_set(&counter, 0);
    atomic_set(&total_reads, 0);
    atomic_set(&total_writes, 0);

    ret = misc_register(&counter_misc_device);
    if (ret) {
        pr_err("counter_driver: misc_register failed\n");
        return ret;
    }

    pr_info("counter_driver: /dev/counter registered\n");

    proc_entry = proc_create("counter_info", 0444, NULL,
                             &counter_proc_fops);

    if (!proc_entry) {
        misc_deregister(&counter_misc_device);
        pr_err("counter_driver: failed to create /proc/counter_info\n");
        return -ENOMEM;
    }

    pr_info("counter_driver: /proc/counter_info created\n");

    return 0;
}

static void __exit counter_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    misc_deregister(&counter_misc_device);

    pr_info("counter_driver: module unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);
