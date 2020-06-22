#!/bin/sh

tcpdump_ver="tcpdump-4.9.2"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-hisiv500-linux-"
output_path="$(pwd)/$tcpdump_ver/build"
shell -e

# Fetch Sources
if [ ! -d $tcpdump_ver ]; then
	mkdir $tcpdump_ver
    wget http://www.tcpdump.org/release/${tcpdump_ver}.tar.gz -O ${tcpdump_ver}.tar.gz
	tar xf ${tcpdump_ver}.tar.gz
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $tcpdump_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh configure $pri_cflags

# make & install
make && make install


# Note:
# shell -e, exit shell if any cmd return not 0