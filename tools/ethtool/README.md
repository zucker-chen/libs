
# ethtool 
Ethtool是Linux下用于查询及设置网卡参数的命令

## 下载安装编译方法
`sh build-ethtool.sh`  
默认pc编译，如果要交叉编译则修改build-ethtool.sh  
`enable_cross_compile="enable"`  

## 使用方法
```
./strace/build/sbin/ethtool eth0
```

