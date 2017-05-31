##
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# 目标名
SET(TARGET_NAME "crypto")

# 头文件路径添加
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# 库路径添加
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

#AUX_SOURCE_DIRECTORY(. BIN_SRC_LIST)
# BASE64
SET(BASE64_SRC_LIST test_base64.c base64.c)
# 执行文件生成
ADD_EXECUTABLE(test_base64 ${BASE64_SRC_LIST})

# DES
SET(DES_SRC_LIST test_des.c des.c)
# 执行文件生成
ADD_EXECUTABLE(test_des ${DES_SRC_LIST})

# DESB
SET(DESB_SRC_LIST test_desb.c desb_crypt.c des.c base64.c)
# 执行文件生成
ADD_EXECUTABLE(test_desb ${DESB_SRC_LIST})