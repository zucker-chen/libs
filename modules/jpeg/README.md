
# libjpeg jpeg的图片库
支持jpeg图片的编码解码

## 下载安装编译方法
默认pc编译，如果要交叉编译则修改build-jpeg.sh  
`enable_cross_compile="enable"`  

## 运行
```
`cd test; sh make.sh`  
`./build/ttf_test`  
`1.jpg` 是源文件  
`2.jpg` 是1.jpg解码成rgb数据然后再叠加字符串后的图片  
`3.bmp` 是在`1.jpg`上叠加字符串的位图图片  
  
```
