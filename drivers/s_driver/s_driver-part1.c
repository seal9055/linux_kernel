/*
 * @file	s_driver.c
 * @author	seal9055
 * @date	26 August 2021
 * @version	1.0
 * @brief	Introductory linux character kernel driver that implements basic functionality. 
 *              This driver will be expanded/improved in future versions.
 *              Blogpost describing functionality at: https://seal9055.com/blog/kernel/char_driver_part-1
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define BUFSIZE 512
#define NAME "s_driver"
#define NUM_MINORS 1
#define FIRST_MINOR 1
#define MESSAGE "Hello World\n"

struct s_driver_dev_data {
        struct cdev cdev;
        char buffer[BUFSIZE];
        size_t size;
}devs[NUM_MINORS];

int major;

/*
 * Open function
 * This function is called whenever the open syscall is invoked onto a device that this driver handles.
 * Returns 0 on success.
 */
static int s_open(struct inode *inode, struct file *file)
{
        struct s_driver_dev_data *data;
        data = container_of(inode->i_cdev, struct s_driver_dev_data, cdev);

        file->private_data = data;

   	printk(KERN_ALERT "Device opened\n");

   	return 0;
}

/*
 * Release function
 * This function is called when close() is called onto the last instance of the device.
 * Returns 0 on success.
 */
static int s_release(struct inode *inode, struct file *file)
{
    	printk(KERN_ALERT "All device's closed\n");
    	return 0;
}

/*
 * Read function
 * This function is called whenever the read syscall is invoked onto a device that this driver handles.
 * Returns number of bytes read.
 */
static ssize_t s_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset)
{
        struct s_driver_dev_data *data = (struct s_driver_dev_data *) file->private_data;
        size_t len = min((size_t)(data->size - *offset), size);

        if (len <= 0)
                return 0;

        if (copy_to_user(ubuf, data->buffer + *offset, len))
                return -EFAULT;
        *offset += len;

    	printk(KERN_ALERT "Device read\n");
    	return len;
}

/*
 * Write function
 * This function is called whenever the write syscall is invoked onto a device that this driver handles.
 * Returns number of bytes written.
 */
static ssize_t s_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset)
{
        struct s_driver_dev_data *data = (struct s_driver_dev_data *) file->private_data;
        size_t len = min((size_t)(data->size - *offset), size);

        if (len <= 0)
                return 0;

        if (copy_from_user(data->buffer + *offset, ubuf, len))
                return -EFAULT;
        *offset += len;
        
        printk(KERN_ALERT "Device written\n");
   	return len;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open    = s_open,
   	.read    = s_read,
   	.write   = s_write,
   	.release = s_release
};

/*
 * Initialization function
 * This function is called once when the driver is initialized, and is then discarded.
 * It starts by registering the driver to a dynamically chosen major number using alloc_chrdev_region().
 * Next it initializes the devices memory to "Hello World\n".
 * Finally it initializes cdev using our fops struct and adds the device to the system using cdev_add,
 * making it live immediately. 
 * Returns 0 on success.
 */
static int __init init_func(void)
{
	dev_t dev = 0;
    	int i;

	if (alloc_chrdev_region(&dev, FIRST_MINOR, NUM_MINORS, NAME) < 0) { 
		printk(KERN_WARNING "Registration failed\n");
		return 1;
	}
	major = MAJOR(dev);

        for (i = 0; i < NUM_MINORS; i++) {
                printk(KERN_ALERT "Create device using mknod /dev/%s%d c %d %d\n", NAME, i, major, i);

                memcpy(devs[i].buffer, MESSAGE, sizeof(MESSAGE));
                devs[i].size = BUFSIZE;

                cdev_init(&devs[i].cdev, &fops);
                cdev_add(&devs[i].cdev, MKDEV(major, i), 1);
        }

        printk(KERN_ALERT "Module successfuly initialized\n");
        return 0;
}

/*
 * Exit function
 * This function is invoked once the driver is unloaded.
 * It handles all necessary cleanup to cleanly remove the driver from the kernel.
 */
static void __exit exit_func(void)
{
        int i;

        for (i = 0; i < NUM_MINORS; i++) {
                cdev_del(&devs[i].cdev);
        }
	
	unregister_chrdev_region(MKDEV(major, FIRST_MINOR), NUM_MINORS);

   	printk(KERN_ALERT "Module successfuly unloaded\n");
}

/*
 * Macros that define general information and the init and exit functions
 */
MODULE_AUTHOR("seal9055 <seal9055@gmail.com>");
MODULE_DESCRIPTION("Linux Character Device Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
module_init(init_func);
module_exit(exit_func);
