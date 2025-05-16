obj-m += stepmotor.o

KDIR := /home/cuong/Desktop/BBB_build/kernelbuildscripts/KERNEL

CROSS_COMPILE := /home/cuong/Desktop/BBB_build/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-
ARCH := arm

PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) clean