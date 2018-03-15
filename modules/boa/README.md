
# boa 
BOA 服务器是一个小巧高效的web服务器，是一个运行于unix或linux下的，支持CGI的、适合于嵌入式系统的单任务的http服务器

## 下载安装编译方法
`sh build-boa.sh`  
默认pc编译，如果要交叉编译则修改build-boa.sh  
`enable_cross_compile="enable"`  

## 交叉编译选项配置
`sh configure --host=arm-linux CC=arm-linux-uclibcgnueabi-gcc --prefix=$(pwd)/build`  
### boa源码编译会保存，需修改文件解决
```
修改compat.h文件  
将下面一行（大概是120行）  
#define TIMEZONE_OFFSET(foo) foo##->tm_gmtoff  
改为：  
#define TIMEZONE_OFFSET(foo) (foo)->tm_gmtoff  
重新make就会通过编译！  
```

## 使用方法
### 创建运行目录
```
mkdir -p test/bin  # test 是boa使用的工作目录，如果是嵌入式设备，放入对应的目录文件夹即可
mkdir -p test/etc  
mkdir -p test/www  
cp boa-0.94.13/sr/boa test/bin  
cp /etc/mime.types test/etc	# 可以找一个精简的类型文件mime.types  
cp boa-0.94.13/boa.conf test/etc  
cp index.html test/www	# 随便找一个index.html即可，实在不行locate index.html搜索一个出来copy就行  
```
### 修改boa.conf
```
User nobody  改为   User root  
Group nogroup  改为   Group root  
* 以下配置需根据实际情况对应修改
AccessLog /var/log/boa/access_log  改为  AccessLog /dev/null  
DocumentRoot /var/www  改为  DocumentRoot /usr/local/boa/www  
MimeTypes /etc/mime.types   MimeTypes /usr/local/boa/etc/mime.types  
ErrorLog /var/log/boa/error_log 改为 ErrorLog /dev/null
```


## 运行
```
test/bin/boa -c test/etc  

浏览器访问测试  
http://127.0.0.1/index.html  
如果浏览器显示是boa/www/index.html内容，则boa环境搭建成功  
```
