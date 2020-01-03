



CROSS_COMPILE=arm-himix200-linux-
ARCH=arm
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld

MODULE_NAME := lens
$(MODULE_NAME)-objs:=lensTimer.o lensDrv.o
obj-m := $(MODULE_NAME).o

KDIR:=/home/zucker/Project/1.hisi/1.sdk/4.Hi3516CV500_SDK_V2.0.1.0/osdrv/opensource/kernel/linux-4.9.y
MAKE:=make  
OSAL_ROOT:=/home/zucker/Project/1.hisi/1.sdk/4.Hi3516CV500_SDK_V2.0.1.0/smp/a7_linux/osal
EXTRA_CFLAGS += -I$(KDIR)/include -I$(OSAL_ROOT)/include

default:  
	$(MAKE) -C $(KDIR) M=$(PWD) modules  ARCH=$(ARCH)
	echo $(CFLAG)
clean:  
	$(MAKE) -C $(KDIR) M=$(PWD) clean  ARCH=$(ARCH)








