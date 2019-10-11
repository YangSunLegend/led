/***********************
 * 用户态的led测试
 * $>led01 /dev/myled allon
 * $>led01 /dev/myled alloff
 * $>led01 /dev/myled on <0~3>
 * $>led01 /dev/myled off <0~3>
 * Author: syy 0126
 * Date: 2017-06-03
 ************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LED_TYPE	'L'
#define LED_ALLON	_IO(LED_TYPE, 1)
#define LED_ALLOFF	_IO(LED_TYPE, 2)
#define LED_ON		_IOW(LED_TYPE, 3, int)
#define LED_OFF		_IOW(LED_TYPE, 4, int)

int main(int argc, char *argv[])
{
	int ret, value, fd;

	if (argc<3 || argc>4) {
		printf("Usage: %s /dev/xxx allon|alloff|on|off <led编号> \n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("打开设备%s失败\n", argv[1]);
		exit(1);
	}

	if (strncmp(argv[2], "allon", 5)==0) {
		ioctl(fd, LED_ALLON, 0);
		printf("将所有灯点亮\n");
	}
	else if (strncmp(argv[2], "alloff", 6)==0) {
		ioctl(fd, LED_ALLOFF, 0);
		printf("将所有灯熄灭\n");
	}
	else if (strncmp(argv[2], "on", 2)==0) {
		value = atoi(argv[3]);
		ret = ioctl(fd, LED_ON, value);
		if (ret)
			printf("点亮LED%d失败\n", value);
		else 
			printf("点亮LED%d成功\n", value);
	}
	else if (strncmp(argv[2], "off", 3)==0) {
		value = atoi(argv[3]);
		ret = ioctl(fd, LED_OFF, value);
		if (ret)
			printf("熄灭LED%d失败\n", value);
		else 
			printf("熄灭LED%d成功\n", value);
	}
	else {
		printf("只支持allon|alloff|on|off\n");
		exit(1);
	}

	close(fd);
	exit(0);
}

