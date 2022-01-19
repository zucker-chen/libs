
# opencv图像处理软件库
它轻量级而且高效——由一系列 C 函数和少量 C++ 类构成，同时提供了Python、Ruby、MATLAB等语言的接口，实现了图像处理和计算机视觉方面的很多通用算法。

## 下载安装编译方法
默认pc编译，如果要交叉编译则修改build-opencv.sh  
`enable_cross_compile="enable"`  

## Tips
* 已验证Linux PC 和 ARM（hi3519v101）编译  
* 默认编译脚本做了功能裁剪，发现某些功能无法使用需要修改重新编译，比如png图片无法解析等  

## 运行
参考demo下的用例

