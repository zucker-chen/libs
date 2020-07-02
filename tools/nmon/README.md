
#  
nmon 开源性能监控工具，用于监控linux系统的资源消耗信息，并能把结果输出到文件中，然后通过nmon_analyser工具产生数据文件与图形化结果。

## 下载安装编译方法
* nmon源码文件已经下载好，直接编译即可  
* nmon依赖ncurses，所以需要先编译ncurses    
* 编译方法：`sh build-nmon.sh`，默认同时会编译ncurses    

## 使用方法
* ./nmon
* 在上面的交互式窗口中，可以使用nmon 快捷键来显示不同的系统资源统计数据：  
```
    q : 停止并退出 Nmon
    h : 查看帮助
    c : 查看 CPU 统计数据
    m : 查看内存统计数据
    d : 查看硬盘统计数据
    k : 查看内核统计数据
    n : 查看网络统计数据
    N : 查看 NFS 统计数据
    j : 查看文件系统统计数据
    t : 查看高耗进程
    V : 查看虚拟内存统计数据
    v : 详细模式
```
* nmon命令参数：  
```
     -f 参数:生成文件,文件名=主机名+当前时间.nmon

     -T 参数:显示资源占有率较高的进程

     -s 参数:-s 10表示每隔10秒采集一次数据

     -c 参数:-c 10表示总共采集十次数据

     -m 参数:指定文件保存目录
```

## Tips
* 在嵌入式端执行`./nmon`时报错：`Error opening terminal: vt102.`
```
    1, 将ncurses-5.9/install/share/terminfo/v/目录下的vt100和vt102下载到目标板的/tmp/terminfo/v/目录中；
    2，设置环境变量：`export  TERM=vt100`, `export  TERMINFO= /tmp/terminfo`
```
