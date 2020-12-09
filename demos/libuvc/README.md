
# libuvc
uvc usb摄像头应用库  

# 抓拍录像实现基础
* 参考[csete/uvccapture](https://github.com/csete/uvccapture.git)，实现抓拍录像功能，方便分析UVC录像的实现流程
* 参考[lintwins/uvc-gadget](https://github.com/lintwins/uvc-gadget.git)，设备实现USB UVC功能，方便定义UVC主从模块的兼容接口定义  
* 参考海思UVC sample(uvc_app/uvc-gadget.c)，与上一uvc-gadget区别：把相关依赖内核头文件结构体独立出来，方便独立实现模块化  

# 设备实现UVC功能
* 参考[lintwins/uvc-gadget](https://github.com/lintwins/uvc-gadget.git)，设备实现USB UVC功能，方便定义UVC主从模块的兼容接口定义  
* 参考海思UVC sample(uvc_app/uvc-gadget.c)，与上一uvc-gadget区别：把相关依赖内核头文件结构体独立出来，方便独立实现模块化  
* 在瑞芯微RV1126下进行编译测试存在问题，系瑞芯微SDK不够完善导致（反馈uvc_app是由另一个ai camera sdk维护）  
	* 上电执行`usb_gadget.sh`，需要插上usb才能成功，不然内核会报错卡死  
	* 执行移植好的程序（用leaf工程RV1126下测试）内核报“空指针访问”错误，内核卡死，触发点是：调用`VIDIOC_STREAMON`方法后卡死  
	* 用瑞芯微自带的uvc_app测试也会失败，内核报错  
* 源码说明：
	* uvc_open设备名需要指定（最好加自动分析模块，分析/dev/videoxx，xx可以根据`/sys/class/video4linux/video%d/name`的内容是否有`usb`或`gadget`字段判断），参考瑞芯微`uvc_app`方法  
	* 不同的视频数据需要修改`uvc_video.c`对应的取流方法  
	* `UAC`音频方法可参考海思`UVC sample(uvc_app/`对应UAC实现  
	
# TEST
* make  
* ./test_uvc_cap  
* ./test_uvc_output    





