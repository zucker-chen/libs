
# zlib 
zlib 是通用的压缩库，提供了一套 in-memory 压缩和解压函数

## 下载安装编译方法
`sh build-zlib.sh`  
默认pc编译，如果要交叉编译则修改build-iperf.sh  
`enable_cross_compile="enable"`  

## 使用方法
* zlib编译出来生成libz.a 和对应头文件
* 生成的库和头文件路径为`zlib-1.2.11/build/lib`和`zlib-1.2.11/build/include`

## 配置文件修改
* make 是报错'undefined reference to rpl_malloc'  
修改config.h 中把 #define malloc rpl_malloc 注释掉   

