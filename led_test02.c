/****************************
 * led的char驱动例子2
 * 用unlocked_ioctl来控制LED的亮灭；
 * 增加proc文件，在用户态获得灯的状态；
 * 为了避免用户态多个人同时操作灯，可以考虑加锁；
 * 用miscdevice注册；
 * 需要在用户态写app来进行测试；
 * Author: syy
 * Date: 2017-05-02
 *****************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h> //可选
#include <linux/ioport.h>
#include <linux/io.h>

//定义LED的相关信息
#define LED_NUM		4
#define GPIO_BASE	0x11000000
#define GPIO_SIZE	0x1000
#define GPM4CON		0x2E0
#define GPM4DAT		0x2E4

//定义ioctl命令
//LED_ON/LED_OFF命令的参数为灯的编号，0～3
#define LED_TYPE	'L'
#define LED_ALLON	_IO(LED_TYPE, 1)
#define LED_ALLOFF	_IO(LED_TYPE, 2)
#define LED_ON		_IOW(LED_TYPE, 3, int)
#define LED_OFF		_IOW(LED_TYPE, 4, int)

//定义驱动的变量
static void __iomem *vir_base;
static int led_state[LED_NUM]; //1:亮；0:灭
struct mutex led_lock; //可选，可用于确保同一时间只有一个人控制灯

//proc文件的读函数
//在函数中根据led_state，向用户态返回LED灯的当前状态
static int
led_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int i, ret = 0;
	ret = sprintf(page, "===LED设备信息===\n");
	for (i=0; i<LED_NUM; i++) {
		ret += sprintf(page+ret, "LED%d当前状态为: %s\n",
			i, led_state[i] ? "亮" : "灭");
	}
	return ret;
}

//本例open/release操作默认成功，可以不写
//不支持read/write，只支持通过ioctl控制
static long
led_ioctl(struct file *filp, unsigned int req, unsigned long arg)
{
	int value, i;
	switch (req) {
	case LED_ALLON:
		for (i=0; i<LED_NUM; i++) {
			value = readl(vir_base+GPM4DAT);
			value &= ~(0x1<<i);
			writel(value, (vir_base+GPM4DAT));
			led_state[i] = 1;
		}
		break;
	case LED_ALLOFF:
		for (i=0; i<LED_NUM; i++) {
			value = readl(vir_base+GPM4DAT);
			value |= (0x1<<i);
			writel(value, (vir_base+GPM4DAT));
			led_state[i] = 0;
		}
		break;
	case LED_ON: //arg为要控制的灯的编号(0~3)，应确保arg的值不能超过LED_NUM
		if (arg >= LED_NUM) {
			printk("灯的编号应该为0~%d\n", LED_NUM-1);
			return -1;
		}
		value = readl(vir_base+GPM4DAT);
		value &= ~(0x1<<arg);
		writel(value, (vir_base+GPM4DAT));
		led_state[arg] = 1;
		break;
	case LED_OFF:
		if (arg >= LED_NUM) {
			printk("灯的编号应该为0~%d\n", LED_NUM-1);
			return -1;
		}
		value = readl(vir_base+GPM4DAT);
		value |= (0x1<<arg);
		writel(value, (vir_base+GPM4DAT));
		led_state[arg] = 0;
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
	int ret=0, value, i;
	//1.ioremap
	vir_base = ioremap(GPIO_BASE, GPIO_SIZE);
	if (!vir_base) {
		printk("映射地址%#x失败\n", GPIO_BASE);
		ret = -EIO;
		goto err_map;
	}

	//2.初始化LED硬件以及全局变量
	value = readl(vir_base+GPM4CON);
	value &= ~(0xFFFF);
	value |= (0x1111);
	writel(value, (vir_base+GPM4CON));
	//灯默认灭
	for (i=0; i<LED_NUM; i++) {
		led_state[i] = 0;
		value = readl(vir_base+GPM4DAT);
		value |= (0x1<<i);
		writel(value, (vir_base+GPM4DAT));
	}
	//mutex_init(&led_lock);

	//3.创建proc文件
	file = create_proc_entry("ledinfo", 0444, NULL);
	if (!file) {
		printk("创建/proc/ledinfo错误\n");
		ret = -1;
		goto err_proc;
	}
	file->read_proc = led_proc_read;

	//4.注册miscdevice
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
	iounmap(vir_base);
err_map:
	return ret;
}

static void __exit my_exit(void)
{
	//注销miscdevice
	//删除proc文件
	//iounmap
	misc_deregister(&led_misc);
	remove_proc_entry("ledinfo", NULL);
	iounmap(vir_base);
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

