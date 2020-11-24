
# iptables 
Linux 内核集成的 IP 信息包过滤系统,该系统有利于在 Linux 系统上更好地控制 IP 信息包过滤和防火墙配置

## 下载安装编译方法
`sh build-iptables.sh`  
默认pc编译，如果要交叉编译则修改build-iperf.sh  
`enable_cross_compile="enable"`  

## 使用方法
* 编译的目标文件`iptables-1.8.6/build/sbin/xtables-legacy-multi`需要软链接或重命名为iptables再使用

## 注意
* 改方法依赖内核支持，如果内核相关服务为打开会报错：  
``` BASH
# ./iptables -L INPUT
modprobe: can't change directory to '4.19.111': No such file or directory
iptables v1.8.6 (legacy): can't initialize iptables table `filter': Table does not exist (do you need to insmod?)
Perhaps iptables or your kernel needs to be upgraded.
```   

