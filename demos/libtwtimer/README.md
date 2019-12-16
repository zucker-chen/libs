
# twtimer
* time whell timer - 时间定时器池。  
* 普通定时器方法setitimer和sleep会冲突，因为它们都使用了信号，而信号会打断sleep。  
* `twtimer_msleep()`使用select机制，不会产生额外`SIGALRM`信号，做了相应信号处理，不会中断sleep。  

# 实现基础
* http://www.cs.columbia.edu/~nahum/w6998/papers/sosp87-timing-wheels.pdf  
* 定时器精度64ms

# Usage
* 实现了2套方法接口    
* 第一套接口：生成1个定时器池句柄，需要管理该句柄，详见test_twtimer.c的test1()函数实现    
* 注意：该接口方法`timer->expire`需要配合`time_wheel_create(clock)`的`clock`，如`timer->expire = clock+5000ms`设置一个5s定时器  
* 第二套接口：自己管理定时器句柄，使用方法类似内核定时器，详见test_twtimer.c的test2()函数实现    
* 注意：该接口方法`timer->expire`直接复制定时器时间即可，如`timer->expire = 5000ms`设置一个5s定时器  
* 尽量使用`twtimer_msleep()`替代`sleep()/usleep()`函数，有效防止sleep被信号中断提前退出  

# TEST
* make clean && make && ./test_twtimer  



