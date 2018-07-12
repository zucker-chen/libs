##
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# 目标名
SET(TARGET_NAME "ttf_test")

# 头文件路径添加
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/include)

# 库路径添加
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib)
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/lib)

# 源码文件列表添加
#AUX_SOURCE_DIRECTORY(. BIN_SRC_LIST)
SET(BIN_SRC_LIST ${TARGET_NAME}.c)

# 执行文件生成
ADD_EXECUTABLE(${TARGET_NAME} ${BIN_SRC_LIST})

target_link_libraries(${TARGET_NAME} -lSDL2 -lSDL2_ttf -lfreetype -lm -lrt -ldl -lpthread)


# Tips
# 1, "SDL_ttf.c:619: undefined reference to `FT_Load_Glyph'"
#    -lfreetype must after -lSDL2_ttf
# 2, "libfreetype.a(ftgzip.o): In function `ft_gzip_file_done'"
#    just add -lz