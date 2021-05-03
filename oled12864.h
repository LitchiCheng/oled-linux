#ifndef _OLED_12864_H_
#define _OLED_12864_H_

#include <linux/ioctl.h>
#include <linux/types.h>

typedef struct oled_start_position{
    char x;
    char y;
}startPositon;

typedef struct oled_string_data{
    startPositon p;
    char* data;
    char len;
}stringData;

typedef struct oled_point_data{
    startPositon p;
    int on;
}pointData;

/* 定义幻数 */
#define OLED_IOC_MAGIC  'k'

/* 定义命令 */
#define OLED_IOC_OPEN   _IO(OLED_IOC_MAGIC, 1)
#define OLED_IOC_CLOSE _IO(OLED_IOC_MAGIC, 2)
#define OLED_IOC_SET_POINT _IOR(OLED_IOC_MAGIC, 3, int)
#define OLED_IOC_SET_STRING _IOW(OLED_IOC_MAGIC, 4, int)
#define OLED_IOC_REFRESH _IO(OLED_IOC_MAGIC, 5)
#define OLED_IOC_CLEAR _IO(OLED_IOC_MAGIC, 6)

#define OLED_IOC_MAXNR 6

#endif  //_OLED_12864_H_
