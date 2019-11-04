
#  
gdb linux下调试工具

## 下载安装编译方法
* `sh build-gdb.sh`  
* 目标文件在gdb/build/bin下  
* gdb编译依赖termcap库，可用ncurses库代替，安装脚本使用ncurses代替方法


## gdb+gdbserver调试方法
* 目标板  
    * gdbserver运行需要调试的程序  
    * `./gdbserver 192.168.1.168:1234 hello`  # hello 是需要调试的程序，需要加'-g'编译  
* PC端  
    * `gdb hello`  
    * `target remote 192.168.1.168:1234`    # 连接到目标板 成功后就可以进行调试  
* 主要  
    * `(gdb)continue or c`      # 这里不能用 run，因为gdbserver已经启动的程序
