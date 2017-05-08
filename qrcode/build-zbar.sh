#!/bin/sh

zbar_ver="zbar-0.10"
enable_cross_compile="enable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="$(pwd)/$zbar_ver/build"

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

cd $zbar_ver

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static
			--without-imagemagick --without-gtk --without-qt --without-python --with-x --disable-video
			--without-libiconv-prefix --without-jpeg --without-xv --without-xshm"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"

# make & install
make -j4 && make install

