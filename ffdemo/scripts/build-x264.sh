#!/bin/sh

enable_cross_compile=1
output_path="./build"

# Fetch Sources
git clone http://git.videolan.org/git/x264.git x264
cd x264

# Build libx264
if [ $enable_cross_compile = 1 ]; then
	cprefix="--cross-prefix=arm-fullhan-linux-uclibcgnueabi-"
	cross_pri_cflags="--host=arm-linux $cprefix"
fi

# ./configure
sh configure $cross_pri_cflags --prefix="./build" --enable-shared --enable-strip	#--enable-static

# make & install
make -j4 && make install

