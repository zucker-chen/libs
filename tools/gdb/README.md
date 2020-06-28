
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
    * `arm-himix200-linux-gdb hello`  
    * `target remote 192.168.1.168:1234`    # 连接到目标板 成功后就可以进行调试  
* 主要  
    * `(gdb)continue or c`      # 这里不能用 run，因为gdbserver已经启动的程序

    
## Tips  
* 32bit/64bit gdb+gdbserver调试时失败(已解决)  
    *  错误信息：  
```
(gdb) target remote 192.168.40.143:1234
Remote debugging using 192.168.40.143:1234
warning: Architecture rejected target-supplied description
Remote 'g' packet reply is too long: 0000000050ffffbe000000000000000000000000000000000000000000000000000000000000000000000000000000000000000070feffbe000000009c1effb6100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
(gdb) 
```    
    * 修改源码解决方法(注意clean之后，remote.c将会重新生成)：  
```
    -+ remote.c
    
  /* Further sanity checks, with knowledge of the architecture.  */
  //if (buf_len > 2 * rsa->sizeof_g_packet)
  //  error (_("Remote 'g' packet reply is too long: %s"), rs->buf);
  if (buf_len > 2 * rsa->sizeof_g_packet) {
    rsa->sizeof_g_packet = buf_len ;
    for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    {
      if (rsa->regs[i].pnum == -1)
        continue;

      if (rsa->regs[i].offset >= rsa->sizeof_g_packet)
        rsa->regs[i].in_g_packet = 0;
      else
        rsa->regs[i].in_g_packet = 1;
    }
  }
```
    * 修改完remote.c源码后会有新的报错：
```
(gdb) target remote 192.168.40.143:1234
Remote debugging using 192.168.40.143:1234
warning: Can not parse XML target description; XML support was disabled at compile time
Reading /lib/ld-uClibc.so.0 from remote target...
warning: File transfers from remote targets can be slow. Use "set sysroot" to access files locally instead.
warning: `target:/lib/ld-uClibc.so.0': Shared library architecture unknown is not compatible with target architecture i386.
Reading /lib/ld-uClibc.so.0 from remote target...
warning: `target:/lib/ld-uClibc.so.0': Shared library architecture unknown is not compatible with target architecture i386.
Reading symbols from target:/lib/ld-uClibc.so.0...(no debugging symbols found)...done.
0x00000000 in ?? ()
```
    * architecture unknown is not compatible with target architecture i386. 解决：  
```
编译时需要制定gdb的--target变量，告诉gdb支持哪个平台CPU的解析，如：`sh configure --target=arm-himix200-linux`   
```

