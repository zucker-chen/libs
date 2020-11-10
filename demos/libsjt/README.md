
# libsjt
struct与json互转模块  

# 支持
* 1，支持`bool, int, char, short, int64, float, double, string`数据类型    
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



# 接口使用说明
基本用法参考`test.sjt.c`用例

#### `sjt_##TYPE(char *str_json, int d, TYPE *val)`
* 该函数实现c语言结构体与json字符串的相互转换功能  
* 该函数使用宏定义生成，通过配套的结构体及成员初始化宏完成，如：  
	* SJT_STRUCT  --> 对应C语言的struct关键字  
	* SJT_FIELD   --> 初始化结构体成员为`bool, int, char, short, int64, float, double`和自定义结构体类型的变量  
	* SJT_STRING  --> 初始化结构体成员为`string`的变量  
	* SJT_ARRAY   --> 初始化成员为数组类型变量  
* TYPE支持`bool, int, char, short, int64, float, double, string`数据类型  
* str_json是对应的json字符串  
* d是选择转换方向，`SJT_STRUCT2JSON`或`SJT_STRUCT2JSON`

#### `sjt_object_*` 系列接口
该方法可以解决`sjt_##TYPE`接口的不足（当struct结构体成员需要放图片等大数据的时候不太好用，因为结构体定义的时候就需要确定空间），而`sjt_object_*` 函数可针对json字符串增删改指定成员。
###### `sjt_object_parser_t *sjt_object_parser_create(char *in_str_json);`
* 初始化函数，对传进来的json字符串进行初始化，方便后面增删改
###### `int sjt_object_get_content(sjt_object_parser_t *sop, char *data, int *size);`
* 获取json指定tag内容，tag可以是`cJSON_Number, cJSON_String, cJSON_Array, cJSON_Object`  
* 使用该函数需要先初始化sop对应的成员`sop->tag_depth, sop->tag_info[sop->tag_depth-1].name`，tag_depth是tag的深度，如果tag是数组，tag_depth+1，数组对应的成员也要tag_depth+1；  
* 如果sop->tag_depth=0则获取的是整个json数据，可以在增删改后使用  
* `cJSON_Number`对应的数据类型有`bool, int, char, short, float, double`很多种，所以拿到字符串后需要自行格式化转换`sscanf`  
* 获取的tag的内容（字符串）通过data/size输出  
###### `int sjt_object_set_content(sjt_object_parser_t *sop, char *data, int size);`
* 增删改json指定tag内容，tag可以是`cJSON_Number, cJSON_String`，cJSON_Object可扩展  
* 可以增删改json中已存在的tag，会用新的内容替换已有的  
* 需要通过sop指定增删改tag的节点类型  
* `cJSON_Number`在cJSON里面是用`double`类型存储，所以data是一个double类型的变量地址，占8byte  

#### `int sjt_object_parser_destroy(sjt_object_parser_t *sop);`
* 反初始化函数，释放相关资源  
