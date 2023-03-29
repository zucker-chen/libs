#!/bin/sh

iperf_ver="iperf-3.12"
enable_cross_compile="enable"	# enable/disable
cross_prefix="aarch64-linux-gnu-"
output_path="$(pwd)/$iperf_ver/build"
shell -e

# Fetch Sources
if [ ! -f ${iperf_ver}.tar.gz ]; then
	mkdir $iperf_ver
	wget https://github.com/esnet/iperf/archive/${iperf_ver#*iperf-}.tar.gz -O ${iperf_ver}.tar.gz
	tar xf ${iperf_ver}.tar.gz
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $(tar -tf ${iperf_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static --disable-shared --without-sctp --without-openssl"
sh configure $pri_cflags

# make & install
make && make install


# Note:
# ${iperf_ver#*iperf-}, cut off 'iperf-' from $iperf_ver
# shell -e, exit shell if any cmd return not 0
# cpp_type_traits.h error: redefinition of ‘struct std::__is_integer<int>’ 
# ---> configure 注释掉 #define bool int