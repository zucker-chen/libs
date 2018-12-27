
# libmsgq
消息队列库封装，方便后续代码复用.

# 实现基础
* 基于system V方式实现的消息队列  
* 基于源码修改:https://github.com/gozfree/libraries/libipc/msgq_sysv.c

# libmsgq Usage
* 其中会涉及到2个`KEY` `MSGQ_KEY1 MSGQ_KEY2`，服务端接受指令用，另一个是服务端处理完指令的返回结果用  
* 客户端及服务端均支持消息队处理回调（即收到消息后触发回调函数）  
* 如果客户端不想用回调处理的方式，只需要`mq_init_client`时最后一个参数NULL即可，此时`mq_recv`手动接收消息  
* linux系统下命令查看操作消息队列命令`ipcs` `ipcrm`  

# TEST
* make  
* ./test_libmsgqs  
* ./test_libmsgqc  



