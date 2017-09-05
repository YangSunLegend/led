/**************************
 * LED的char驱动例子1
 * 将4个LED识别为一个硬件进行控制；
 * 支持用write进行控制
 * 用户态测试方法：
 * $>echo on|off >/dev/myled 
 * echo on则4个灯全亮；echo off则4个灯全灭；
 * 用miscdevice注册
 * Author: syy
 * Date: 2017-06-28
 ***************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/io.h>

#define LED_NUM		4
#define GPIO_BASE	0x11000000
#define GPIO_SIZE	0x1000
#define GPM4CON		0x2E0
#define GPM4DAT		0x2E4

//定义需要的变量
static void __iomem *vir_base;

//open/release函数如果返回0，可以不写
//用户态的open/close操作默认成功

//file_ops->write
//对应用户态的echo on|off >/dev/myled
static ssize_t
led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int value;
	char tmp[10] = {0};
	if (copy_from_user(tmp, buf, 3))
		return -EFAULT;
	if (strncmp(tmp, "on", 2)==0) {
		value = readl(vir_base+GPM4DAT);
		value &= ~(0xF<<0);
		writel(value, (vir_base+GPM4DAT));
	}
	else if (strncmp(tmp, "off", 3)==0) {
		value = readl(vir_base+GPM4DAT);
		value |= (0xF<<0);
		writel(value, (vir_base+GPM4DAT));
	}
	else {
		printk("Only support on|off\n");
		return -1;
	}
	return count;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write,
};
static struct miscdevice led_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "myled", //创建设备文件/dev/myled
	.fops	= &led_fops,
};

static int __init my_init(void)
{
	int value,ret;
	//1.映射虚拟地址
	vir_base = ioremap(GPIO_BASE, GPIO_SIZE);
	if (!vir_base) {
		printk("无法映射地址0x%x\n", GPIO_BASE);
		return -EIO;
	}
	//2.将GPIO配置为输出
	value = readl(vir_base+GPM4CON);
	value &= ~(0xFFFF);
	value |= (0x1111);
	writel(value, (vir_base+GPM4CON));
	//3.注册miscdevice
	ret = misc_register(&led_misc);
	if (ret) {
		printk("注册LED设备失败\n");
		iounmap(vir_base);
		return ret;
	}
	printk("注册LED设备成功\n");
	return 0;
}
static void __exit my_exit(void)
{
	misc_deregister(&led_misc);
	iounmap(vir_base);
}
module_init(my_init);
module_exit(my_exit);
MODULE_AUTHOR("SYY");
MODULE_LICENSE("GPL");
