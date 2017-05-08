
# strace 
一款非常强大的调试用户程序的工具

## 下载安装编译方法
`sh build-strace.sh`  
默认pc编译，如果要交叉编译则修改build-strace.sh  
`enable_cross_compile="enable"`  

## 交叉编译选项配置
`sh configure --host=arm-linux --target=arm-linux CC=arm-linux-uclibcgnueabi-gcc --prefix=$(pwd)/build`  

## 使用方法
```
./strace/build/bin/strace ps
```

## 配置文件修改
* sudo apt-get install libtool  
不然报错："Can't exec "libtoolize": No such file or directory"  
* configure.ac  
注释"AM_EXTRA_RECURSIVE_TARGETS"行，不然会报错："warning: macro `AM_EXTRA_RECURSIVE_TARGETS' not found in library"  

