#ifndef DEV_DRIVER_H
#define DEV_DRIVER_H

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#define COMPATIBLE      "td3_jyl"
#define CLASS_NAME      "td3_jyl_class"
#define MENOR           0
#define CANT_DISP       1

static int i2c_remove(struct platform_device * i2c_pd);
static int i2c_probe(struct platform_device * i2c_pd);
static int change_permission_cdev(struct device *dev, struct kobj_uevent_env *env);
static void __exit i2c_exit(void);
static int __init i2c_init(void);

static int open_jyl(struct inode *inode, struct file *file);
static int release_jyl(struct inode *inode, struct file *file);
static ssize_t read_jyl(struct file * device_descriptor, char __user *user_buffer, size_t read_len, loff_t * my_loff_t);
static ssize_t write_jyl(struct file * device_descriptor, const char __user *user_buffer, size_t read_len, loff_t * my_loff_t);
static long ioctl_jyl(struct file *file, unsigned int cmd, unsigned long arg);

irqreturn_t jyl_i2c_irq_handler(int irq, void *dev_id, struct pt_regs *regs);

static void i2c_softreset(void);

static struct {
    dev_t myi2c;

    struct cdev * myi2c_cdev;
    struct device * myi2c_device;
    struct class * myi2c_class;
}state ;

static struct of_device_id i2c_of_device_ids [] = {
    {
        .compatible = COMPATIBLE,
    }, {}
};

static struct platform_driver i2c_pd = {
    .probe = i2c_probe,
    .remove = i2c_remove,
    .driver = {
        .name = COMPATIBLE,
        .of_match_table = of_match_ptr(i2c_of_device_ids)
    },
};

struct file_operations i2c_ops = {
    .owner = THIS_MODULE,
    .open  = open_jyl,
    .read  = read_jyl,
    .write = write_jyl,
    .release = release_jyl,
    .unlocked_ioctl = ioctl_jyl
};

struct {
	uint32_t    tx_index;
	uint32_t    tx_len;
	uint32_t    tx_data;
	uint8_t * 	tx_buff;

	uint32_t    rx_index;
	uint32_t    rx_len;
	uint32_t    rx_data;
    uint8_t * 	rx_buff;

} data_i2c;

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Joaquin Yanez Landa");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("TD3_I2C_MODULE");

#endif /*DEV_DRIVER_H*/