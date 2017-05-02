#!/bin/sh

valgrind_ver="valgrind-3.12.0"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="$(pwd)/$valgrind_ver/build"

# Fetch Sources
if [ ! -d $valgrind_ver ]; then
	mkdir $valgrind_ver
	wget http://valgrind.org/downloads/${valgrind_ver}.tar.bz2
	tar xf ${valgrind_ver}.tar.bz2
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $valgrind_ver

# Fix configure ( "armv7*)" to "armv7*|arm)" )
#sed -n "/armv7/s/armv7\*)/armv7\*|arm)/p" ./configure 
sed -i "/armv7/s/armv7\*)/armv7\*|arm)/" ./configure 

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh configure $pri_cflags

# make & install
make -j4 && make install


# export VALGRIND_LIB="/mnt/xxx/build/lib/valgrind"
# ./valgrind ls -l