/****************************
 * led的char驱动例子3
 * 功能及使用同led_test02.c；
 * 用GPIO库来控制LED；不再直接访问寄存器；
 * Author: syy
 * Date: 2017-05-04
 *****************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h> //可选
//去掉寄存器访问需要的头文件，加入GPIO库的头文件
#include <linux/gpio.h> //request_gpio gpio_set|get_value
#include <plat/gpio-cfg.h> //s3c_gpio_cfgpin
#include <mach/gpio.h> //GPIO号，EXYNOS4X12_GPM4()

//定义LED的相关信息
#define LED_NUM		4

//定义ioctl命令
//LED_ON/LED_OFF命令的参数为灯的编号，0～3
#define LED_TYPE	'L'
#define LED_ALLON	_IO(LED_TYPE, 1)
#define LED_ALLOFF	_IO(LED_TYPE, 2)
#define LED_ON		_IOW(LED_TYPE, 3, int)
#define LED_OFF		_IOW(LED_TYPE, 4, int)

//定义驱动的变量，数组中记录4个LED对应的GPIO号
static int led_gpios[LED_NUM] = {
	EXYNOS4X12_GPM4(0),
	EXYNOS4X12_GPM4(1),
	EXYNOS4X12_GPM4(2),
	EXYNOS4X12_GPM4(3),
};


//proc文件的读函数
//在函数中调用gpio_get_value，获得GPIO当前输出的电平
//向用户态返回LED灯的当前状态
static int
led_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int i, ret = 0;
	ret = sprintf(page, "===LED设备信息===\n");
	for (i=0; i<LED_NUM; i++) {
		ret += sprintf(page+ret, "LED%d当前状态为: %s\n",
			i, gpio_get_value(led_gpios[i]) ? "灭":"亮");
	}
	return ret;
}

static long
led_ioctl(struct file *filp, unsigned int req, unsigned long arg)
{
	//用gpio_set_value来设置GPIO的输出
	int i;
	switch (req) {
	case LED_ALLON:
		for (i=0; i<LED_NUM; i++)
			gpio_set_value(led_gpios[i], 0);
		break;
	case LED_ALLOFF:
		for (i=0; i<LED_NUM; i++)
			gpio_set_value(led_gpios[i], 1);
		break;
	case LED_ON:
		if (arg >= LED_NUM) {
			printk("灯的编号应该为0~%d\n", LED_NUM-1);
			return -1;
		}
		gpio_set_value(led_gpios[arg], 0);
		break;
	case LED_OFF:
		if (arg >= LED_NUM) {
			printk("灯的编号应该为0~%d\n", LED_NUM-1);
			return -1;
		}
		gpio_set_value(led_gpios[arg], 1);
		break;
	default:
		printk("不支持命令号%#x\n", req);
		return -1;
	}
	return 0;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
};
static struct miscdevice led_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "myled",
	.fops	= &led_fops,
};

static int __init my_init(void)
{
	struct proc_dir_entry *file;
	int ret=0, i, j;

	//用GPIO库实现GPIO的初始化
	for (i=0; i<LED_NUM; i++) {
		ret = gpio_request(led_gpios[i], "myio");
		if (ret) {
			printk("申请gpio%d失败\n", led_gpios[i]);
			goto err_req;
		}
		s3c_gpio_cfgpin(led_gpios[i], S3C_GPIO_OUTPUT);
		gpio_set_value(led_gpios[i], 1);
	}

	file = create_proc_entry("ledinfo", 0444, NULL);
	if (!file) {
		printk("创建/proc/ledinfo错误\n");
		ret = -1;
		goto err_proc;
	}
	file->read_proc = led_proc_read;

	ret = misc_register(&led_misc);
	if (ret) {
		printk("注册led设备失败\n");
		goto err_misc;
	}
	printk("注册led设备成功\n");
	return ret;
err_misc:
	remove_proc_entry("ledinfo", NULL);
err_proc:
err_req:
	for (j=0; j<i; j++) 
		gpio_free(led_gpios[j]);
	return ret;
}

static void __exit my_exit(void)
{
	int i;
	misc_deregister(&led_misc);
	remove_proc_entry("ledinfo", NULL);
	for (i=0; i<LED_NUM; i++)
		gpio_free(led_gpios[i]);
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

