
#  
tinyproxy 是一个http代理服务器

## 下载安装编译方法
`sh build-tinyproxy.sh`  
默认pc编译，如果要交叉编译则修改该脚本 
`enable_cross_compile="enable"`  

### 编译时修改(build-tinyproxy.sh 中用sed命令修改)
* 修改Makefile支持交叉编译  


## uchdpc使用方法
* 拷贝default.script到板级，如 `cp udhcp-0.9.8/samples/simple.script default.script`  
* 执行：udhcpc -b -i eth0 -s default.script  

## TIPS
* 正向代理、反向代理区别
	* 正向代理，比如翻墙访问www.google.com，一般浏览器需要配置好代理服务器地址及端口（也就是：客户机需要告诉代理服务器的目标IP及端口）  
	* 反向代理，比如方位www.taobao.com，真实的访问资源在不同的内网服务器上（对客户机不用管内部资源IP及端口）
	
