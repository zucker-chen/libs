#!/bin/sh

zbar_ver="zbar-0.10"
libiconv_ver="libiconv-1.14"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
libiconv_output_path="$(pwd)/$libiconv_ver/build"
zbar_output_path="$(pwd)/$zbar_ver/build"

# Fetch Sources
if [ ! -d $libiconv_ver ]; then
	mkdir $libiconv_ver
	wget http://ftp.gnu.org/pub/gnu/libiconv/${libiconv_ver}.tar.gz
	tar xf ${libiconv_ver}.tar.gz
fi
# Fetch Sources
if [ ! -d $zbar_ver ]; then
	mkdir $zbar_ver
	wget http://downloads.sourceforge.net/project/zbar/zbar/0.10/zbar-0.10.tar.bz2
	tar xf ${zbar_ver}.tar.bz2
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# Compile libiconv
cd $libiconv_ver
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$libiconv_output_path --enable-static"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"

# make & install
make -j4 && make install
cd -

# Compile zbar
cd $zbar_ver
# "./libtool: eval: line 961: syntax error near unexpected token `|'"
export NM=nm

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$zbar_output_path --enable-static
			--without-imagemagick --without-gtk --without-qt --without-python --disable-video
			--without-jpeg --without-xv --without-xshm --without-x
			CPPFLAGS=-I$libiconv_output_path/include LDFLAGS=-L$libiconv_output_path/lib"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"

# make & install
make -j4 && make install



# Note:
# libiconv must profile ==> "zbar/qrcode/qrdectxt.c:9:19: error: iconv.h: No such file or directory"
# libiconv-1.15 ==> "getprogname.c:147:4: error: #error "getprogname module not ported to this OS""
# 