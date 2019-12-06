
# gpioconfig
海思3516CV500下的GPIO驱动实现, 海思其他方案稍作修改就行  

# function
* gpio get/set  
	* 海思SDK会有文档描述GPIO驱动使用方法，参考：`外围设备驱动 操作指南.doc`
	* 这里我们使用linux内核标准的gpio_request方法

* timer  
	* 参考海思SDK模块源码实现：`Hi3516CV500_SDK_V2.0.1.0/smp/a7_linux/osal"/osal_timer.c`  
	* osal_timer.c中默认最小定时器精度为ms，需要修改成us  
	* 提高定时器精度：内核的默认滴答精度为10ms，即定时器精度为10ms，配置内核参数修改为1ms： Kernel Features --> Timer frequency (1000 Hz)   
* gpio模拟pwm  
	* 使用定时器实现，目前实现4路PWM模拟  

# TEST
* make  
* `arm-himix200-linux-gcc iotest.c -o iotest`  
* insmode gpioconfig.ko  
* ./iotest 1 0x406 1	// 设置GPIO4_6为高  

