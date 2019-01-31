#!/bin/bash


BUILD_DIR="build"

if [ ! -d $BUILD_DIR ];then
	mkdir $BUILD_DIR
fi

cd $BUILD_DIR
	cmake ../ -DCMAKE_TOOLCHAIN_FILE=../../cmake/common-build-env.cmake
	[ $? != 0 ] && return 1;
	make -j4
cd -

echo "#### Cmake done."
echo "#### If you want 'make' or 'make clean' manual, pls 'cd build' first, then do it."


