
# strace 
一款测试最大TCP/UDP网络带宽工具

## 下载安装编译方法
`sh build-iperf.sh`  
默认pc编译，如果要交叉编译则修改build-iperf.sh  
`enable_cross_compile="enable"`  

## 使用方法
```
server:
./strace/build/bin/iperf -s -p 5001
client:
./strace/build/bin/iperf -c 127.0.0.1 -p 5001
```

## 配置文件修改
* make 是报错'undefined reference to rpl_malloc'  
修改config.h 中把 #define malloc rpl_malloc 注释掉   

