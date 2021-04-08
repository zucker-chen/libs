
#  
udhcp 是一个字符编码转换库

## 下载安装编译方法
`sh build-udhcp.sh`  
默认pc编译，如果要交叉编译则修改该脚本 
`enable_cross_compile="enable"`  

### 编译时修改(build-udhcp.sh 中用sed命令修改)
* 修改Makefile支持交叉编译  
* 修改dhcpc.c，解决case中没有break的错误  

## uchdpc使用方法
* 拷贝default.script到板级，如 `cp udhcp-0.9.8/samples/simple.script default.script`  
* 执行：udhcpc -b -i eth0 -s default.script  

## udhcpd使用方法
* 拷贝udhcpd.conf到板级，如`cp udhcp-0.9.8/samples/udhcpd.conf udhcpd.conf` 
* 修改udhcpd.conf文件，指定网络设备如eth0  
* mkdir -p /var/lib/misc/ &&  touch /var/lib/misc/udhcpd.leases  
* 执行： udhcpd /etc/udhcpd.conf & 

