#!/bin/sh

zlib_ver="iptables-1.8.6"
enable_cross_compile="enable"	# enable/disable
cross_prefix="arm-linux-gnueabihf-"	#"arm-hisiv500-linux-"
output_path="$(pwd)/$zlib_ver/build"


# Fetch Sources
if [ ! -d $zlib_ver ]; then
	git clone git://git.netfilter.org/iptables $zlib_ver	
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $zlib_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --disable-nftables --enable-static --disable-shared"
sh autogen.sh
sh configure $pri_cflags

# make & install
make && make install


# Note:
# shell -e, exit shell if any cmd return not 0
# unknown option: --host=arm-linux ===> 所有交叉编译参数用export进行传递即可