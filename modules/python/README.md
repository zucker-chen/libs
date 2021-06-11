
# python
比较流行的脚本语言

## 下载安装编译方法
默认pc编译，如果要交叉编译则修改build-openssl.sh  
`enable_cross_compile="enable"`  

## 使用方法
```
链接到对应的库及头文件使用即可
./build/lib
./build/include
```

## 板端运行
* 把交叉编译好的目标文件和库目录拷贝到设备
	* Python-3.6.12/build/bin/python3.6
	* Python-3.6.12/build/lib/python3.6  (此目录可以做裁剪)
* 设备端添加环境变量PYTHONHOME、PYTHONPATH
	* export PYTHONHOME=$PATH
	* export PYTHONPATH=/mnt/ev200/python-build/lib/python3.6  
* 运行测试
	* ./python3.6

## Tips
* 交叉编译时需要保证ubuntu的环境python版本比需要编译的版本新，所以如果ubuntu的python不够新，可以用该源码编译个PC版本，步骤：  
	* ./configure -prefix=/usr/  
	* make  
	* sudo make install  
	* ln -s /usr/bin/python3.6 /usr/bin/python   
	* python -V  
* 报错“subprocess.CalledProcessError: Command ‘(‘lsb_release’, ‘-a’)’ returned non-zero exit status 1.”  
	* 解决：rm -rf /usr/bin/lsb_release 后重新编译即可  
* 交叉编译的--host应该是arm-himix100-linux， arm-linux会报错	
* 板端运行报错“Could not find platform independent libraries <prefix>”
	* 解决：export PYTHONHOME=$PATH
* 板端运行报错“ModuleNotFoundError: No module named 'encodings'”
	* 解决：由于python找不到正确的库路径，添加库路径环境变量后正常 export PYTHONPATH=/mnt/ev200/python-build/lib/python3.6  
