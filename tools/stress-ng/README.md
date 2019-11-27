
#  
stress-ng 系统压力测试工具，包括CPU/IO/内存

## 下载安装编译方法
`sh build-iconv.sh`  
默认pc编译，如果要交叉编译则修改该脚本 
`enable_cross_compile="enable"`  

## 目标文件
`stress-ng-0.10.11/build/usr/bin/stress-ng`  

## 使用方法

* 指定CPU负载  
```
开启一个线程，使CPU达到80%负载
stress-ng -c 1 -l 80
```

* CPU 密集型进程（使用CPU的进程）  
```
对IO进行压测(使用stress观测到的iowait指标可能为0，所以使用stress-ng)
[root@nginx ~]# stress-ng -i 4 --hdd 1 --timeout 600
 
[root@nginx ~]# uptime
11:11:12 up  1:05,  4 users,  load average: 4.35, 4.11, 3.65
 
[root@nginx ~]# mpstat -P ALL 5
Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    0.20    0.00   13.04   38.70    0.00    1.33    0.00    0.00    0.00   46.73
Average:       0    0.07    0.00    6.63   40.96    0.00    3.72    0.00    0.00    0.00   48.62
Average:       1    0.19    0.00   20.14   26.77    0.00    0.04    0.00    0.00    0.00   52.85
Average:       2    0.27    0.00   13.81   45.15    0.00    0.88    0.00    0.00    0.00   39.89
Average:       3    0.27    0.00   11.22   42.20    0.00    0.80    0.00    0.00    0.00   45.51
 
[root@nginx sysstat-12.1.5]# pidstat -u 5
 
1.通过uptime可以观察到，系统平均负载很高，通过mpstat观察到2个CPU使用率很高，平均负载也很高，而iowait为0，说明进程是CPU密集型的；
2.是由进程使用CPU密集导致系统平均负载变高、CPU使用率变高; 
3.可以通过pidstat查看是哪个进程导致CPU使用率较高
```

* 大量进程的场景（等待CPU的进程->进程间会争抢CPU）  
```
模拟16个进程，本机是4核
[root@nginx ~]# stress -c 16 --timeout 600
```
