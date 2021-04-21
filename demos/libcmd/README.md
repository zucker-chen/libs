
# libcmd
程序调试命令模块，满足程序开发是对程序状态进行查询及控制目的，最常用是telnet通过命令开启程序调试信息  

# 实现基础
* 用到libmsg()库及list.h  
* 该模块分为主从模式，主（即主程序）负责接收（消息队列）处理发过来的指令，从（客户端程序，类似busybox，其他指令名用软链接方式指令执行），如终端输入`v-cmd-show-all`即显示目前所有支持的调试指令  
* 目前主要考虑一个主程序，多个从命令调用，如果要实现多对多需要做一些处理(需要用到多组消息队列)  
* 其中使用到2个消息队列ID(KEY)，`msgid_s`负责服务端接收指令用(C->S)，主从约定好的；`msgid_c`负责客户端接受指令用(S->C)，主从握手时协商的临时ID  

# TEST-1
* make  
* ./test_libcmds  #或创建注册的指令软链接到./test_libcmdc  
* ./v-cmd-tty-dump # 换一个窗口执行，此时会将主程序的调试信息打印到当前终端    
* ./v-cmd-args-test 1 2 3 4 5 6 7 8   # 测试指令传参效果，最大支持8个参数，每个参数最大16字符，默认最后一个参数是当前终端设备名(/dev/pts/3)，如果当前参数8个则第9个参数是终端名，如果当前参数是3个则4个参数是终端名  

# TEST-2
* make
* ./test_libcmd  # 该程序包括了server 和 client模块，只需要启动一个进程即可  
* ././v-cmd-tty-dump # 换一个窗口执行，此时会将主程序的调试信息打印到当前终端    

# 问题1
* 问题：出现关闭当前tty，再新建tty（telnet终端），开启调试信息重定向后发现printf答应没办法输出  
* 分析：  
	* 出问题条件都是由于当前进程的STDOUT_FILENO对应tty已经不存在引起，如果已经不存在后再进行dup2进行重定向STDOUT_FILENO就会出现异常，反而STDERR_FILENO不会出问题  
	* 进一步测试发现printf与fprintf(stdout, xxx)没有输出，用fprintf(fdopen(STDOUT_FILENO, "w+"), xxx)则能正常输出；  
* 解决：进行dup2进行STDOUT_FILENO重定向后把stdou句柄更新一下后正常（stdout = fdopen(STDOUT_FILENO, "w+");）  

