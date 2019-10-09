
# thread_pool
* 线程池，线程过多会带来调度开销，进而影响缓存局部性和整体性能。  
* 而线程池维护着多个线程，等待着监督管理者分配可并发执行的任务。  
* 这避免了在处理短时间任务时创建与销毁线程的代价。  

# 实现基础
* 基于源码修改:https://github.com/happyfish100/libfastcommon/tree/master/src/pthread_pool.c

# thread_pool Usage
* 第一步：`threadpool_init(20);`设置线程池核心最大线程数  
* 第二步：`threadpool_run(thread_cb, &arg[i], name);`创建线程任务，并执行任务，如果当前线程池已满则阻塞等待  
* 第三步：`threadpool_destroy();`销毁线程池  

# TEST
* make  
* ./test_pthread_pool  



