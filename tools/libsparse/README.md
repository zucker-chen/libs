
# libsparse 
libsparse原本是安卓系统里面用的ext4分区镜像`sparse`格式的解析，里面的`img2simg/simg2img`可轻松将`sparse`与`RAW`格式互转

## 下载安装编译方法
`make`  
默认pc编译，如果要交叉编译则修改`Makefile`  
`CROSS_PREFIX` 打开并配置交叉编译工具链即可 

## 使用方法
`img2simg/simg2img`可轻松将`sparse`与`RAW`格式互转

## 注意
交叉编译会提示找不到-lz，zlib库，需要手动交叉编译改库并拷贝相关库和头文件到该目录后进行编译，zlib库的编译路径：[../zlib]