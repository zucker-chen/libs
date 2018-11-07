
# sunitest - single file unit test
单文件代码模块(简单实用代码用例)

## 目录
* **test_ttyS0.c**  
串口信息重定向输出相关处理  
`./test_ttyS0 on`：将控制终端打印信息定向到当前tty（即telnet）显示  
`./test_ttyS0 off`：恢复控制终端为"/dev/console"(即串口)  

* **getopts.sh**  
脚本参数输入处理，支持格式类似：
`sh getopts.sh -a 1 -b "" -v file.txt`  


* **getopts.c**  
C语言输入参数处理，测试结果：
```
./build/bin/getopts -p 1 -c s2l33m -s imx323 -m 0 -v 6.1.10.1
pack mode = 1
chip = s2l33m
sensor = imx323
misc = 0
version = 6.1.10.1
```

* **test_mdio.c**  
读写PHY寄存器工具，使用方法：  
read:    `./test_mdio eth0 0x02`  
write:   `./test_mdio eth0 0x0 0x12`  

* **ringbuf.c**  
循环缓冲用例，测试方法  
```
./build/bin/ringbuf  
```
特性：  
  1，支持1写入N读取，默认N=32  
  2，暂时未用锁，并未充分测试，需跟实际使用情况分析  
  3，稍作改动即可用于共享内存的循环缓冲  
  4，该循环缓冲主要用于数据包大小变化的码流  

