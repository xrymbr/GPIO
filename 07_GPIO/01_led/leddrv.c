#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>



static int major = 0;
static struct class *led_class;

static struct gpio_desc *led2_gpio;


/* 3. 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t led_gpiod_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* write(fd, &val, 1); */
static ssize_t led_gpiod_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	char status;
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_from_user(&status, buf, 1);

	/* 根据次设备号和status控制LED */
	gpiod_set_value(led2_gpio, status);
	
	return 1;
}

static int led_gpiod_open (struct inode *node, struct file *file)
{
	//int minor = iminor(node);
	
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 根据次设备号初始化LED */
	gpiod_direction_output(led2_gpio, 0);
	
	return 0;
}

static int led_gpiod_release (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}


static const struct file_operations led_gpiod_fops = {
	.owner   = THIS_MODULE,
	.open    = led_gpiod_open,
	.read    = led_gpiod_read,
	.write   = led_gpiod_write,
	.release = led_gpiod_release,
};





static int led_drv_probe(struct platform_device *pdev)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	major = register_chrdev(0, "led_gpio", &led_gpiod_fops);
	led2_gpio = gpiod_get(&pdev->dev, "led2",GPIOD_OUT_LOW);
	
	led_class = class_create(THIS_MODULE, "led_gpio_class");
	if (IS_ERR(led_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "led_gpio");
		gpiod_put(led2_gpio);
		return PTR_ERR(led_class);
	}

	device_create(led_class, NULL, MKDEV(major, 0), NULL, "led_gpio%d", 0); /* /dev/100ask_led0 */
	
	return 0;
}
static int led_drv_remove(struct platform_device *pdev)
{

	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	device_destroy(led_class, MKDEV(major, 0));
	class_destroy(led_class);
	unregister_chrdev(major, "led_gpio");
	gpiod_put(led2_gpio);
	return 0;
}



static const struct of_device_id led_drv_of_match[] = {
	{ .compatible = "gpio_led_test", },
	{ },
};

static struct platform_driver led_drv = {
	.probe		= led_drv_probe,
	.remove		= led_drv_remove,
	.driver		= {
		.name	= "gpio_led",
		.of_match_table = led_drv_of_match,
	}
};


/* 1. 入口函数 */
static int __init led_drv_init(void)
{	
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 1.1 注册一个platform_driver */
	return platform_driver_register(&led_drv);
}


/* 2. 出口函数 */
static void __exit led_drv_exit(void)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 2.1 反注册platform_driver */
	platform_driver_unregister(&led_drv);
}

module_init(led_drv_init);
module_exit(led_drv_exit);

MODULE_LICENSE("GPL");






