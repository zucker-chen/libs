# cmake模版及用例

## 模版文件说明
**cmake**
├── CMakeLists.txt.binlib.txt
├   编译执行程序带库CMakeLists模版(工程模块以库形式编译时选用)
├── CMakeLists.txt.bin.txt
├   编译执行程序不带库CMakeLists模版(工程模块以源码形式直接编译时选用)
├── CMakeLists.txt.lib.txt
├   编译库CMakeLists模版(动态库和静态库编译时选用)
├── amba-toolchain.cmake
├   安霸平台交叉编译环境cmake模版
├── fh-toolchain.cmake
├   富瀚平台交叉编译环境cmake模版
├── vatics-m3c-toolchain.cmake
├   睿智平台交叉编译环境cmake模版
├── common-build-env.cmake
├   编译选项相关cmake模版(顶级模版必选)
├── cmake_command_list.txt
└   cmake常用命令说明

## 安装
* ### 1. 是否交叉编译环境
    否，可不配置工具链环境(默认即可)，配置CMakeLists.txt
    是，跳转到2
* ### 2. 配置交叉编译环境
    选择对应平台的cmake模版，修改编译器路径及名称，如`amba-toolchain.cmake`
    修改`common-build-env.cmake`打开`INCLUDE(${CMAKE_MODULE_PATH}/amba-toolchain.cmake)`
* ### 3. 编辑CMakeLists.txt
    根据工程情况(编译库还是执行文件等)拷贝CMakeLists.txt到源码目录，根据需要可修改

## 编译
`sh make.sh`
编译结果(相应的inc lib bin文件)默认放在`./build`目前对应目录下

