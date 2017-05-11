#!/bin/sh

enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# ====> libiconv Build
libiconv_ver="libiconv-1.14"
output_path="$(pwd)/$libiconv_ver/build"
# Fetch Sources
if [ ! -d $libiconv_ver ]; then
	wget http://ftp.gnu.org/pub/gnu/libiconv/${libiconv_ver}.tar.gz
	tar xf ${libiconv_ver}.tar.gz
fi
cd $libiconv_ver
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install
cd -


# Note:
# libiconv must profile ==> "zbar/qrcode/qrdectxt.c:9:19: error: iconv.h: No such file or directory", "libiconv-1.14" is ok
# libiconv-1.15 ==> "getprogname.c:147:4: error: #error "getprogname module not ported to this OS"", "libiconv-1.14" is ok.

