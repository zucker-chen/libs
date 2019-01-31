##
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# 目标名
SET(TARGET_NAME "test_record")

# 头文件路径添加
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../libffmpeg/include)

# 库路径添加
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lib)
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../libffmpeg/lib)

# 源码文件列表添加
#AUX_SOURCE_DIRECTORY(. BIN_SRC_LIST)
SET(BIN_SRC_LIST ${TARGET_NAME}.c record_files_mng.c crc32.c)

# 执行文件生成
ADD_EXECUTABLE(${TARGET_NAME} ${BIN_SRC_LIST})

target_link_libraries(${TARGET_NAME} -lavformat -lavcodec -lswresample -lavutil -lm -lrt -ldl -lpthread)
