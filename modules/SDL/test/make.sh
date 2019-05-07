#!/bin/sh

# 创建外部库目录
ext_dir="ext"
# SDL
tmp_dir=$ext_dir/sdl/include
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp -r ../SDL-*/build/include/SDL*/* $tmp_dir && cp -r ../SDL_*/build/include/SDL*/*.h $tmp_dir
tmp_dir=$ext_dir/sdl/lib
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir  && cp ../SDL-*/build/lib/*.a $tmp_dir && cp ../SDL_*/build/lib/*.a $tmp_dir
# freetype
tmp_dir=$ext_dir/freetype/include
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp -r ../freetype2*/build/include/freetype2/* $tmp_dir
tmp_dir=$ext_dir/freetype/lib
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp ../freetype2*/build/lib/*.a $tmp_dir


# cmake build
BUILD_DIR="build"
if [ ! -d $BUILD_DIR ];then
    mkdir $BUILD_DIR
fi

cd $BUILD_DIR
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/common-build-env.cmake
    [ $? != 0 ] && return 1;
    make -j4 
cd -
echo "#### Cmake done."
echo "#### If you want 'make' or 'make clean' manual, pls 'cd build' first, then do it."