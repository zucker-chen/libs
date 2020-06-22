
# tcpdump 
网络抓包工具，类似Wireshark

## 下载安装编译方法
`sh build-libpcap.sh`  
`sh build-tcpdump.sh`  
默认pc编译，如果要交叉编译则修改编译脚本中的变量：
`enable_cross_compile="enable"`  

## 使用方法
```
./strace/build/bin/tcpdump -i eth0

```

## 配置文件修改
1, tcpdump依赖libpcap库，libpcap主要是用来实现tcpdump抓包数据保存为Wireshark可以解析的格式;  
2, libpcap库需要先编译，路径与tcpdump同目录就行，tcpdump编译脚本不需要特殊指明libpcap库的路径; 
3, 编译完后只需要拷贝tcpdump到目标板运行即可，不依赖其他库;  

