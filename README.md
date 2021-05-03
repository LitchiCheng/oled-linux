# oled-linux
linux driver for led

## oled12864
- 接口：4针i2c
- 供电：3.3-5v
- 像素：128*64
- 体积：27*27*2
- ic：SSD1306
- 地址：0x3c（手册中0x78，实际0x3c）

## dts
- 在i2c控制器下追加节点
```c
oled12864:oled12864@3c {
    compatible = "dar,oled12864";
    reg = <0x3c>;
    status = "okay";  /* 如果需要改为okay */
};
```

## make
- 修改KERNELDIR为内核目录
```c
KERNELDIR := /home/dar/Project/linux/kernal_source/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek/
```

## demo
- oled为测试程序，显示数据