
# openssl开源安全套接字层密码库
囊括主要的密码算法、常用的密钥和证书封装管理功能及SSL协议，并提供丰富的应用程序供测试或其它目的使用

## 下载安装编译方法
默认pc编译，如果要交叉编译则修改build-openssl.sh  
`enable_cross_compile="enable"`  

## 使用方法
```
链接到对应的库及头文件使用即可
./build/lib
./build/include
```

## Tips
* 源码下载地址：https://www.openssl.org/source/，github上也可以clone到源码  
* 该脚本适用交叉编译openssl版本：1.0.xx 和 0.9.8x  
* 交叉编译未成功openssl版本：1.1.xx ,x86编译正常  
* CC/AR/RANLIB变量只能修改Makefile，通过make CC=XXX方式编译不成功  
* 交叉编译和PC编译切换，需要make distclean一次  
* 如果指定os/compiler 就只能用./Configure，其他情况Configure和config文件等价(如果使用./config Makefile默认添加-march=pentium)  
