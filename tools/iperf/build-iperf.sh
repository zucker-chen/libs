#!/bin/sh

iperf_ver="iperf-2.0.4-RELEASE"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-hisiv500-linux-"
output_path="$(pwd)/$iperf_ver/build"
shell -e

# Fetch Sources
if [ ! -d $iperf_ver ]; then
	mkdir $iperf_ver
	wget https://github.com/esnet/iperf/archive/${iperf_ver#*iperf-}.tar.gz -O ${iperf_ver}.tar.gz
	tar xf ${iperf_ver}.tar.gz
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $iperf_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh configure $pri_cflags

# comment out #define malloc rpl_malloc, when make error.
sed -i "s/^#define malloc rpl_malloc/\/\*#define malloc rpl_malloc\*\//" config.h

# make & install
make && make install


# Note:
# ${iperf_ver#*iperf-}, cut off 'iperf-' from $iperf_ver
# shell -e, exit shell if any cmd return not 0