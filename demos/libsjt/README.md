
# libsjt
struct与json互转模块  

# 支持
* 1，支持`bool, int, char, short, long long, float, double`数据类型    
* 2，long long 到 json 使用string保存，固定为10进制字符串形式  
* 3，支持数组，包括自定义结构体的数组，仅支持一维数组  

# TEST
* make  
* ./test_libsjt  

# 使用步骤  
* 先定义C语言Struct结构体，名字如`test_t`  
* 用`SJT_STRUCT, SJT_FIELD, SJT_STRING, SJT_ARRAY`宏定义，根据C语言结构体重新生成代码  
* 调用`sjt_test_t()`函数即可进行struct到json数据的互转  
* 详见`test_sjt.c`代码用例  



