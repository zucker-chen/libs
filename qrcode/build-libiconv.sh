#!/bin/sh

libiconv_ver="libiconv-1.15"
enable_cross_compile="enable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="$(pwd)/$libiconv_ver/build"

# Fetch Sources
if [ ! -d $libiconv_ver ]; then
	mkdir $libiconv_ver
	https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.15.tar.gz
	tar xf ${libiconv_ver}.tar.bz2
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux-gnueabihf --target=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $libiconv_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
exit
# make & install
make -j4 && make install

