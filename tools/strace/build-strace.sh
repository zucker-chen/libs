#!/bin/sh

strace_ver="strace"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="$(pwd)/$strace_ver/build"

# Fetch Sources
if [ ! -d $strace_ver ]; then
	mkdir $strace_ver
	git clone https://github.com/strace/${strace_ver}.git
	tar xf ${strace_ver}.tar.bz2
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux --target=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $strace_ver

if [ ! -f configure ]; then
	# comment out AM_EXTRA_RECURSIVE_TARGETS
	sed -i "s/^.*AM_EXTRA_RECURSIVE_TARGETS/#AM_EXTRA_RECURSIVE_TARGETS/" configure.ac
	# autoreconf
	sh bootstrap
fi

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh configure $pri_cflags

# make & install
make CFLAGS+="-static" && make install
# strip
${cross_prefix}strip strace


# Note: rely on tools
# sudo apt-get upgrade automake		==> "warning: macro `AM_EXTRA_RECURSIVE_TARGETS' not found in library"
#										"Alternatively, you can comment out the AM_EXTRA_RECURSIVE_TARGETS in configure.ac", ---worked.
# sudo apt-get install libtool		==> "Can't exec "libtoolize": No such file or directory"
# test cmd : ./strace ps