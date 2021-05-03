KERNELDIR := /home/dar/Project/linux/kernal_source/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek/
CURRENT_PATH := $(shell pwd)

obj-m := oled12864.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	date
	$(CC) app/oled12864demo.c -o app/oled12864demo -Wall -pthread -O2

install:
	scp oled12864.ko app/oled12864demo root@192.168.192.5:/home/root

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm app/oled12864demo -rf