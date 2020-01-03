



CROSS_COMPILE=arm-hisiv300-linux-
ARCH=arm
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld

MODULE_NAME := lens
$(MODULE_NAME)-objs:=lensTimer.o  lensMs41908.o lensDrv.o
obj-m := $(MODULE_NAME).o

KDIR:=/home/work/platform/hi3516/source/200w/kernel_svn/linux-3.4.y
MAKE:=make  
EXTRA_CFLAGS += -I$(KDIR)/include -march=armv7-a -Wno-implicit-function-declaration

default:  
	$(MAKE) -C $(KDIR) M=$(PWD) modules  ARCH=$(ARCH)
	echo $(CFLAG)
clean:  
	$(MAKE) -C $(KDIR) M=$(PWD) clean  ARCH=$(ARCH)








