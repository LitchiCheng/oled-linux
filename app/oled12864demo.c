#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "../oled12864.h"

int main(int argc, char *argv[])
{
	int fd;
	int ret = 0;
	int arg = 0;
	int cmd;
	char* filename = "/dev/oled12864";
	fd = open(filename, O_RDWR);
	if(fd < 0) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

    // cmd = OLED_IOC_CLOSE;
    // if (ioctl(fd, cmd, &arg) < 0){
	// 	printf("Call cmd fail\n");
	// 	return -1;
    // }
	// usleep(1000000);

	struct oled_string_data tmp;
	tmp.p.x = 20;
	tmp.p.y = 32;
	char t[] = "i love alice";
	tmp.data = t;
	tmp.len = 12;

	cmd = OLED_IOC_CLEAR;
	if (ioctl(fd, cmd, &tmp) < 0){
		printf("Call cmd fail\n");
		return -1;
	}

	cmd = OLED_IOC_SET_STRING;
	if (ioctl(fd, cmd, &tmp) < 0){
		printf("Call cmd fail\n");
		return -1;
	}

	char p[] = "i am darboy";
	tmp.p.y += 20;
	tmp.data = p;
	cmd = OLED_IOC_SET_STRING;
	if (ioctl(fd, cmd, &tmp) < 0){
		printf("Call cmd fail\n");
		return -1;
	}

	struct oled_point_data tmp1;
	tmp1.p.x = 1;
	tmp1.p.y = 1;
	tmp1.on = 1;

	for(int i = 0;i < 127; i++){
		for(int j = 0;j < 63; j++){
			tmp1.p.x = i+1;
			tmp1.p.y = j+1;
			tmp1.on = 1;
			cmd = OLED_IOC_SET_POINT;
			if (ioctl(fd, cmd, &tmp1) < 0){
				printf("Call cmd fail\n");
				return -1;
			}
			cmd = OLED_IOC_REFRESH;
			if (ioctl(fd, cmd, &tmp1) < 0){
				printf("Call cmd fail\n");
				return -1;
			}
		}
	}

	usleep(100000);
	close(fd);
	return 0;
}

