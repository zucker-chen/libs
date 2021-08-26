
# nginx 
nginx 实现http代理，支持交叉编译方法

## 下载安装编译方法
#### nginx 依赖pcre和openssl库，所以需要先下载编译好这两个库
###### `./build-openssl.sh`
###### `./build-pcre.sh`
#### 交叉编译nginx需要修改一些工作配置文件
###### 修改编译方法
需要修改的文件已经备份到nginx-patch目录，执行`build-nginx.sh`就会自动拷贝修改文件

## 使用方法
编译成功的目标文件及配置文件在 nginx-1.20.1/build 目录下

#TIPS
# 使用Hi3516编译器验证交叉编译成功 

