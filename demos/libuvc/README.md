
# libuvc
uvc usb摄像头应用库  

# 抓拍录像实现基础
* 参考[csete/uvccapture](https://github.com/csete/uvccapture.git)，实现抓拍录像功能，方便分析UVC录像的实现流程
* 参考[lintwins/uvc-gadget](https://github.com/lintwins/uvc-gadget.git)，设备实现USB UVC功能，方便定义UVC主从模块的兼容接口定义  
* 参考海思UVC sample(uvc_app/uvc-gadget.c)，与上一uvc-gadget区别：把相关依赖内核头文件结构体独立出来，方便独立实现模块化  

# 设备实现UVC功能
* 参考[lintwins/uvc-gadget](https://github.com/lintwins/uvc-gadget.git)，设备实现USB UVC功能，方便定义UVC主从模块的兼容接口定义  
* 参考海思UVC sample(uvc_app/uvc-gadget.c)，与上一uvc-gadget区别：把相关依赖内核头文件结构体独立出来，方便独立实现模块化  

# TEST
* make  
* ./test_libuvc  


