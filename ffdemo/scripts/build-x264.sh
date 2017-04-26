#!/bin/sh

# usage: sh build-x264.sh "disable" "enable" "arm-fullhan-linux-uclibcgnueabi-"

# enable/disable
enable_shared_libs=$0 && [ -z $enable_shared_libs ] && enable_shared_libs="disable"			# default enable_shared_libs="disable"	
enable_cross_compile=$2 && [ -z $enable_cross_compile ] && enable_cross_compile="disable"	# default enable_cross_compile="disable"
cross_prefix=$3 && [ -z $cross_prefix ] && cross_prefix="arm-fullhan-linux-uclibcgnueabi-"	# default cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="./build"

# Fetch Sources
[ ! -d x264 ] && git clone http://git.videolan.org/git/x264.git x264
cd x264

# shared libs, default static
if [ "$enable_shared_libs" = "enable" ]; then
	shared_libs_cflags="--enable-shared"
else
	shared_libs_cflags="--enable-static"
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux --cross-prefix=$cross_prefix"
fi

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path $shared_libs_cflags --enable-strip --disable-asm"
echo "sh configure $pri_cflags"
sh configure $pri_cflags

# make & install
make -j4 && make install

echo "#### make install success. output path = x264/build"