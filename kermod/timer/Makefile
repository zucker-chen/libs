
export ARCH=arm
export CROSS_COMPILE=arm-hisiv500-linux-
LINUX_ROOT = /home/zucker/Project/2.sdk/1.Hi3519V101_SDK_V1.0.3.0/osdrv/opensource/kernel/linux-3.18.y
#LINUX_ROOT = /lib/modules/$(shell uname -r)/build

MODULE_NAME := timer
$(MODULE_NAME)-objs := test_timer.o utimer.o 
obj-m := $(MODULE_NAME).o

EXTRA_CFLAGS+=-I$(PWD)/..


default:
	make -C $(LINUX_ROOT) M=$(PWD) modules 
clean: 
	make -C $(LINUX_ROOT) M=$(PWD) clean 



