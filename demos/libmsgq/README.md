
# libmsgq
消息队列库封装，方便后续代码复用.

# 实现基础
* 基于system V方式实现的消息队列  
* 基于源码修改:https://github.com/gozfree/libraries/libipc/msgq_sysv.c

# libmsgq Usage
* 实现逻辑：  
```
1，使用客户-服务模式，客户端上线时需要先绑定（MQ_CMD_BIND），退出再解绑（MQ_CMD_UNBIND）；
2，整过过程只需要一个MSG KEY；
3，使用消息队列的“msg type”进行会话管理；
4，每个客户端与服务端绑定成功后，则会用一对新唯一的msg type，进行双向通信；
5，每个会话用线程管理，可以实现多个客户端并发处理；
```
* Tips：  
```
1，mq_init_server/mq_deinit_client，都可以选择不用回调接受数据；
2，mq_send/mq_recv，手动收发数据，是阻塞模式；
```
* linux系统下命令查看操作消息队列命令`ipcs` `ipcrm`    


# TEST
* make  
* ./test_libmsgqs  
* ./test_libmsgqc  



