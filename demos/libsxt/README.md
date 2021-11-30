
# libsxt
struct与xml互转模块  

# 支持情况
* 1，支持`bool, int, char, short, long long, float, double`数据类型    
* 2，long long 到 json 使用string保存，固定为10进制字符串形式  
* 3，支持数组，包括自定义结构体的数组，仅支持一维数组  
* 4，不支持xml的属性解析，如 `<tt tt="1"/>`，解析不出`tt=1`
* 5，"struct与xml互转" 支持部分成员变量转换（取交集），参考`test_sxt_ro.c`  

# TEST
* make  
* ./test_libsxt  

# 使用步骤  
* 先定义C语言Struct结构体，名字如`test_t`  
* 用`SXT_STRUCT, SXT_FIELD, SXT_STRING, SXT_ARRAY`宏定义，根据C语言结构体重新生成代码  
* 调用`sxt_test_t()`函数即可进行struct到json数据的互转  
* 详见`test_sxt.c`代码用例  



