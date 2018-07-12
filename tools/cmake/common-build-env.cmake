##
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

## 指定cmake模块的路径
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

## 平台工具链环境加载 (需要修改)
#INCLUDE(${CMAKE_MODULE_PATH}/fh-toolchain.cmake)
#INCLUDE(${CMAKE_MODULE_PATH}/vatics-m3c-toolchain.cmake)
#INCLUDE(${CMAKE_MODULE_PATH}/amba-toolchain.cmake)
#INCLUDE(${CMAKE_MODULE_PATH}/hisi-toolchain.cmake)

## 编译选项
SET(BUILD_DEBUG_TYPE ON)	# ON=debug, OFF=release
IF (BUILD_DEBUG_TYPE)
	SET(FLAGS_ARGS "-O0 -Wall -g -ggdb")
ELSE (BUILD_DEBUG_TYPE)
	SET(FLAGS_ARGS "-O3 -Wall")
ENDIF (BUILD_DEBUG_TYPE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_ARGS} -std=gnu99")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS_ARGS} -std=c++11")

## 静态库or动态库
SET(BUILD_SHARED_LIBS OFF)	# ON=动态库, OFF=静态库

## 编译输出路径配置 (可以修改)
# 输出路径(bin lib inc)
SET(COMMON_OUTPUT_PATH ${PROJECT_BINARY_DIR})	# ./build
#SET(COMMON_OUTPUT_PATH ${PROJECT_SOURCE_DIR})	# ./
# 执行文件输出路径
SET(EXECUTABLE_OUTPUT_PATH ${COMMON_OUTPUT_PATH}/bin)
# 库文件输出路径
SET(LIBRARY_OUTPUT_PATH ${COMMON_OUTPUT_PATH}/lib)
