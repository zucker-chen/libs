
# libringbuf
循环缓冲库模块  

# 实现基础
* 1，支持1写入N读取，默认N=32  
* 2，暂时未用锁，并未充分测试，需跟实际使用情况分析  
* 3，稍作改动即可用于共享内存的循环缓冲  

# TEST
* make  
* ./test_ringbuf  

