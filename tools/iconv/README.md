
#  
libiconv 是一个字符编码转换库

## 下载安装编译方法
`sh build-iconv.sh`  
默认pc编译，如果要交叉编译则修改该脚本 
`enable_cross_compile="enable"`  

## 交叉编译选项配置
`sh configure --host=arm-linux --target=arm-linux CC=arm-linux-uclibcgnueabi-gcc --prefix=$(pwd)/build`  

