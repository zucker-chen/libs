#!/bin/sh

zlib_ver="zlib-1.2.11"
enable_cross_compile="enable"	# enable/disable
cross_prefix="arm-linux-gnueabihf-"	#"arm-hisiv500-linux-"
output_path="$(pwd)/$zlib_ver/build"
shell -e

# Fetch Sources
if [ ! -d $zlib_ver ]; then
	mkdir $zlib_ver
	wget http://www.zlib.net/fossils/${zlib_ver}.tar.gz -O ${zlib_ver}.tar.gz
	tar xf ${zlib_ver}.tar.gz
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
	export $cross_pri_cflags
	cross_pri_cflags=""
fi

cd $zlib_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh configure $pri_cflags

# make & install
make && make install


# Note:
# shell -e, exit shell if any cmd return not 0
# unknown option: --host=arm-linux ===> 所有交叉编译参数用export进行传递即可