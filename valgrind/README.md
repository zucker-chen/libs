
# 本模块内容
valgrind调试工具下载、编译(交叉编译)、测试方法

# 脚本使用方法
```
sh build-valgrind.sh
完成内容：下载valgrind源码、解压、配置、编译(默认PC环境，交叉编译需要修改 build-valgrind.sh)  
```

## valgrind简介
Valgrind是一款用于内存调试、内存泄漏检测以及性能分析的软件开发工具

## 下载
```
wget http://valgrind.org/downloads/valgrind-3.12.0.tar.bz2
```

## 修改配置
修改configure： armv7*) 改成 armv7*|arm)

## 编译环境配置
```
./configure --host=arm-linux CC=arm-none-linux-gnueabi-gcc CPP=arm-none-linux-gnueabi-cpp CXX=arm-none-linux-gnueabi-g++ --prefix=$(pwd)/build
```

## 编译安装
```
make -j4
make install
```

## 测试验证
```
export VALGRIND_LIB="/mnt/xxx/build/lib/valgrind"
./valgrind ls -l
```
### 使用 Valgrind Memcheck
`./valgrind --tool=memcheck ./a.out`
**该工具可以检测下列与内存相关的问题 :**
* 未释放内存的使用
* 对释放后内存的读/写
* 对已分配内存块尾部的读/写
* 内存泄露
* 不匹配的使用malloc/new/new[] 和 free/delete/delete[]
* 重复释放内存



