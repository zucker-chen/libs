
#  
stunnel 一种加密技术，可以实现https转http

## 下载安装编译方法
1，先编译openssl：`sh build-openssl.sh`
2，再编译stunnel：`sh build-stunnel.sh`  
默认pc编译，如果要交叉编译则修改该脚本 
`enable_cross_compile="enable"`  


## stunnel配置文件
``` etc/stunnel.conf
foreground=yes
; TLS front-end to a web server
[https]
accept  = 443
connect = 127.0.0.1:80
cert = /home/zucker/Project/8.debug/libs/tools/stunnel/ssl_crt/server.pem

```

## https证书生成方法
```
1，生成密钥
openssl genrsa -out server.key 4096
2，生成一个证书请求（中间文件），需要拿这个文件生成
openssl req -new -key server.key -subj "/CN=CZQ" -out server.csr # -config openssl.cnf
3，生成自签名CA证书
openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.pem
4，合并key与CA证书（stunnel需要使用合并后的文件）
cat server.key >> server.pem
```

## stunnel
sudo ./stunnel-stunnel-5.60/build/bin/stunnel etc/stunnel.conf  

## TIPS
* 正向代理、反向代理区别
	* 正向代理，比如翻墙访问www.google.com，一般浏览器需要配置好代理服务器地址及端口（也就是：客户机需要告诉代理服务器的目标IP及端口）  
	* 反向代理，比如方位www.taobao.com，真实的访问资源在不同的内网服务器上（对客户机不用管内部资源IP及端口）
* ARM平台运行碰到问题  
	* stunnel+openssl, 目前海思arm平台测试通过的版本是stunnel-5.60 + openssl-1.1.1c，搭配openssl-1.0.2l测试没通过，应该是TLS版本不匹配原因
	* error:140AB18F:SSL routines:SSL_CTX_use_certificate:ee key too small   ==> 生成密钥时的长度该厂4096则正常
	* error:0306E06C:bignum routines:BN_mod_inverse:no inverse   ==> 编译选项中没有指定芯片架构，配置脚本在猜测时将芯片架构猜测为linux-x86_64，解决办法参考build-openssl.sh中的Tips