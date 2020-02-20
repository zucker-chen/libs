
# libringbuf
循环缓冲库模块  

# 实现基础
* 1，支持1写入N读取，默认N=32  
* 2，暂时未用锁，并未充分测试，需跟实际使用情况分析  
* 3，稍作改动即可用于共享内存的循环缓冲  

# TEST
* make  
* ./test_ringbuf  

# Tips
* 可编程数组定义，如果兼容64位系统要做结构体对齐处理  
```C
typedef struct ringbuf_unit_s {
    struct ringbuf_unit_s 	*prev;
    struct ringbuf_unit_s 	*next;
    long 					size;					// Compatible with 64-bit machines
    uint8_t 				data[0];
} ringbuf_unit_t;
// 如果 `size`是`int`类型，再`64bit`系统，`sizeof(size) = 4`，则编译器会做8字节对齐;  
// 但是 `&data`地址是紧跟`size`结束的地址（而不是对齐后的地址），不然`&data`的地址偏移计算得不到你想要的结果.  
```

