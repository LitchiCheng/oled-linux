#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "oled.h"
#include "oledfont.h"
#include "oled12864.h"

#define OLED12864_CNT	1
#define OLED12864_NAME	"oled12864"

struct oled12864_dev {
	dev_t devid;				/* 设备号 	 */
	struct cdev cdev;			/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;		/* 设备 	 */
	struct device_node	*nd; 	/* 设备节点 */
	int major;					/* 主设备号 */
	void *private_data;			/* 私有数据 		*/
};

static struct oled12864_dev oled12864dev;

static int oled12864_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int oled12864_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &oled12864dev; /* 设置私有数据 */
	return 0;
}

static s32 oled12864_write_regs(struct oled12864_dev *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* oled12864地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	return i2c_transfer(client->adapter, &msg, 1);
}

static void oled12864_write_onereg(struct oled12864_dev *dev, u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	oled12864_write_regs(dev, reg, &buf, 1);
}

void OLED_WR_Byte(u8 data, u8 mode)
{
	u8 buf[2] = {};
	if(mode){
		buf[0] = 0x40;
	}else{
		buf[0] = 0x00;
	}
	buf[1] = data;
	oled12864_write_onereg(&oled12864dev, buf[0], buf[1]);
}

/*******************/
u8 OLED_GRAM[144][8];

//开启OLED显示 
void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	OLED_WR_Byte(0x14,OLED_CMD);//开启电荷泵
	OLED_WR_Byte(0xAF,OLED_CMD);//点亮屏幕
}

//关闭OLED显示 
void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	OLED_WR_Byte(0x10,OLED_CMD);//关闭电荷泵
	OLED_WR_Byte(0xAF,OLED_CMD);//关闭屏幕
}

//画点 
//x:0~127
//y:0~63
void OLED_DrawPoint(u8 x,u8 y)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	OLED_GRAM[x][i]|=n;
}

//清除一个点
//x:0~127
//y:0~63
void OLED_ClearPoint(u8 x,u8 y)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	OLED_GRAM[x][i]|=n;
	OLED_GRAM[x][i]=~OLED_GRAM[x][i];
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//size:选择字体 12/16/24
//取模方式 逐列式
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1)
{
	u8 i,m,temp,size2,chr1;
	u8 y0=y;
	size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
	chr1=chr-' ';  //计算偏移后的值
	for(i=0;i<size2;i++)
	{
		if(size1==12)
        {temp=asc2_1206[chr1][i];} //调用1206字体
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //调用1608字体
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //调用2412字体
		else return;
		for(m=0;m<8;m++)           //写入数据
		{
			if(temp&0x80)OLED_DrawPoint(x,y);
			else OLED_ClearPoint(x,y);
			temp<<=1;
			y++;
			if((y-y0)==size1)
			{
				y=y0;
				x++;
				break;
			}
		}
  	}
}

//显示字符串
//x,y:起点坐标  
//size1:字体大小 
//*chr:字符串起始地址 
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1)
{
	while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
	{
		OLED_ShowChar(x,y,*chr,size1);
		x+=size1/2;
		if(x>128-size1)  //换行
		{
			x=0;
			y+=2;
    	}
		chr++;
  	}
}

/**********************/
//更新显存到OLED	
void OLED_Refresh(void)
{
	u8 i,n;
	for(i=0;i<8;i++){
	   	OLED_WR_Byte(0xb0+i,OLED_CMD); //设置行起始地址
	   	OLED_WR_Byte(0x00,OLED_CMD);   //设置低列起始地址
	   	OLED_WR_Byte(0x10,OLED_CMD);   //设置高列起始地址
	   	for(n=0;n<128;n++)
			OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA);
  	}
}

//清屏函数
void OLED_Clear(void)
{
	u8 i,n;
	for(i=0;i<8;i++){
	   for(n=0;n<128;n++){
			OLED_GRAM[n][i]=0;//清除所有数据
		}
  	}
	OLED_Refresh();//更新显示
}

static ssize_t oled12864_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	signed int data[7];
	long err = 0;
	struct oled12864_dev *dev = (struct oled12864_dev *)filp->private_data;
	err = copy_to_user(buf, data, sizeof(data));
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t oled12864_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct oled12864_dev *dev = (struct oled12864_dev *)filp->private_data;
	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	u8 reg = databuf[0];		/* 获取寄存器 */
	oled12864_write_regs(dev, reg, databuf+1, cnt-1);
	OLED_Refresh();//更新显示
	return 0;
}

/*IO操作*/
static int oled12864_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    int ret = 0;
    int ioarg = 0;
    
    /* 检测命令的有效性 */
    if (_IOC_TYPE(cmd) != OLED_IOC_MAGIC) 
        return -EINVAL;
    if (_IOC_NR(cmd) > OLED_IOC_MAXNR) 
        return -EINVAL;

    /* 根据命令类型，检测参数空间是否可以访问 */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err) 
        return -EFAULT;

    /* 根据命令，执行相应的操作 */
    switch(cmd) {
		/* 打印当前设备信息 */
		case OLED_IOC_OPEN:
			OLED_DisPlay_On();
			printk("<--- CMD OLED_IOC_OPEN Done--->\n\n");
			break;

			/* 打印当前设备信息 */
		case OLED_IOC_CLOSE:
			OLED_DisPlay_Off();
			printk("<--- CMD OLED_IOC_CLOSE Done--->\n\n");
			break;

		case OLED_IOC_REFRESH:
			OLED_Refresh();
			break;
		
		case OLED_IOC_CLEAR:
			OLED_Clear();
			break;

		/* 获取参数 */
		case OLED_IOC_SET_POINT: 
			{
				pointData tmp;
				copy_from_user((unsigned char *)&tmp,(unsigned char *)arg,sizeof(stringData));
				// OLED_Clear();
				if(tmp.on = 0){
					OLED_ClearPoint(tmp.p.x, tmp.p.y);
				}else{
					OLED_DrawPoint(tmp.p.x, tmp.p.y);
				}
				
				printk("<--- In Kernel = %d %d %d %d--->\n\n", tmp.p.x, tmp.p.y, tmp.on, 16);
				
			}
			break;
		
		/* 设置参数 */
		case OLED_IOC_SET_STRING:
			{
				stringData tmp;
				copy_from_user((unsigned char *)&tmp,(unsigned char *)arg,sizeof(stringData));
				// OLED_Clear();
				OLED_ShowString(tmp.p.x, tmp.p.y, tmp.data, tmp.len);
				OLED_Refresh();
				printk("<--- In Kernel = %d %d %s %d--->\n\n", tmp.p.x, tmp.p.y, tmp.data, 16);
				
			}
			break;

		default:  
			return -EINVAL;
    }
    return ret;
}

/* oled12864操作函数 */
static const struct file_operations oled12864_ops = {
	.owner = THIS_MODULE,
	.open = oled12864_open,
	.read = oled12864_read,
	.write = oled12864_write,
	.unlocked_ioctl = oled12864_ioctl,
	.release = oled12864_release,
};

void oled12864_reginit(void)
{
	OLED_WR_Byte(0xAE,OLED_CMD);//--turn off oled panel
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WR_Byte(0x81,OLED_CMD);//--set contrast control register
	OLED_WR_Byte(0xCF,OLED_CMD);// Set SEG Output Current Brightness
	OLED_WR_Byte(0xA1,OLED_CMD);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WR_Byte(0xC8,OLED_CMD);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WR_Byte(0xA6,OLED_CMD);//--set normal display
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3f,OLED_CMD);//--1/64 duty
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WR_Byte(0x00,OLED_CMD);//-not offset
	OLED_WR_Byte(0xd5,OLED_CMD);//--set display clock divide ratio/oscillator frequency
	OLED_WR_Byte(0x80,OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WR_Byte(0xD9,OLED_CMD);//--set pre-charge period
	OLED_WR_Byte(0xF1,OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WR_Byte(0xDA,OLED_CMD);//--set com pins hardware configuration
	OLED_WR_Byte(0x12,OLED_CMD);
	OLED_WR_Byte(0xDB,OLED_CMD);//--set vcomh
	OLED_WR_Byte(0x40,OLED_CMD);//Set VCOM Deselect Level
	OLED_WR_Byte(0x20,OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WR_Byte(0x02,OLED_CMD);//
	OLED_WR_Byte(0x8D,OLED_CMD);//--set Charge Pump enable/disable
	OLED_WR_Byte(0x14,OLED_CMD);//--set(0x10) disable
	OLED_WR_Byte(0xA4,OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
	OLED_WR_Byte(0xA6,OLED_CMD);// Disable Inverse Display On (0xa6/a7) 
	OLED_WR_Byte(0xAF,OLED_CMD);
	OLED_Clear();

	OLED_ShowString(20,32,"oled is ready",12);
	OLED_Refresh();
	printk("oled12864 regint................\r\n");
}

static int oled12864_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	/* 1、构建设备号 */
	if (oled12864dev.major) {
		oled12864dev.devid = MKDEV(oled12864dev.major, 0);
		register_chrdev_region(oled12864dev.devid, OLED12864_CNT, OLED12864_NAME);
	} else {
		alloc_chrdev_region(&oled12864dev.devid, 0, OLED12864_CNT, OLED12864_NAME);
		oled12864dev.major = MAJOR(oled12864dev.devid);
	}

	/* 2、注册设备 */
	cdev_init(&oled12864dev.cdev, &oled12864_ops);
	cdev_add(&oled12864dev.cdev, oled12864dev.devid, OLED12864_CNT);

	/* 3、创建类 */
	oled12864dev.class = class_create(THIS_MODULE, OLED12864_NAME);
	if (IS_ERR(oled12864dev.class)) {
		return PTR_ERR(oled12864dev.class);
	}

	/* 4、创建设备 */
	oled12864dev.device = device_create(oled12864dev.class, NULL, oled12864dev.devid, NULL, OLED12864_NAME);
	if (IS_ERR(oled12864dev.device)) {
		return PTR_ERR(oled12864dev.device);
	}

	oled12864dev.private_data = client; /* 设置私有数据 */

	oled12864_reginit();		
	return 0;
}

static int oled12864_remove(struct i2c_client *client)
{
	/* 删除设备 */
	cdev_del(&oled12864dev.cdev);
	unregister_chrdev_region(oled12864dev.devid, OLED12864_CNT);

	/* 注销掉类和设备 */
	device_destroy(oled12864dev.class, oled12864dev.devid);
	class_destroy(oled12864dev.class);
	return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id oled12864_id[] = {
	{"dar,oled12864", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id oled12864_of_match[] = {
	{ .compatible = "dar,oled12864" },
	{ /* Sentinel */ }
};

static struct i2c_driver oled12864_driver = {
	.probe = oled12864_probe,
	.remove = oled12864_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "oled12864",
		   	.of_match_table = oled12864_of_match, 
		   },
	.id_table = oled12864_id,
};
		   
static int __init oled12864_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&oled12864_driver);
	return ret;
}

static void __exit oled12864_exit(void)
{
	i2c_del_driver(&oled12864_driver);
}

module_init(oled12864_init);
module_exit(oled12864_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("darboy");

