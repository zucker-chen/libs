
# memtester 内存测试工具
DDR稳定性

## 下载安装编译方法
`sh build-memtester.sh`  
默认pc编译，如果要交叉编译则修改build-memtester.sh  
`enable_cross_compile="enable"`  

## 使用方法
```
./memtester-4.3.0/build/bin/memtester 10M 1
```

## 脚本修改内容
* 交叉编译选项配置(不需要忽略)
修改conf-cc、conf-ld文件配置交叉编译工具cc  
* 配置make install路径
修改Makefile文件配置输出路径  
