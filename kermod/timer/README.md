
# utimer
utimer user timer 驱动模块，包括普通定时器和高精度定时器  

# 说明 
* 普通定时器（jiffies定时器），依赖于内核`Tick`（一般默认是100Hz），即定时器最大精度10ms  
* hrtimer定时器，高精度定时器，使用硬件中断方式，不依赖内核时钟，但是内核需要打开配置`CONFIG_HIGH_RES_TIMERS=yes`  

# TEST
* make  
* `sudo insmod utimer.ko`  
* `sudo rmmod utimer`  
* dmesg
