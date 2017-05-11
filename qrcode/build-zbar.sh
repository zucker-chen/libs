#!/bin/sh

enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# ====> ZBar Build
zbar_ver="zbar-0.10"
output_path="$(pwd)/$zbar_ver/build"
# Fetch Sources
if [ ! -d $zbar_ver ]; then
	wget http://downloads.sourceforge.net/project/zbar/zbar/0.10/${zbar_ver}.tar.bz2
	tar xf ${zbar_ver}.tar.bz2
fi
cd $zbar_ver

export NM=nm
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static
			--without-gtk --without-qt --without-python --disable-video
			--without-jpeg --without-xv --without-xshm --without-x
			--without-imagemagick --without-libiconv-prefix"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install



# Note:
# --without-imagemagick ==> disable support for scanning images using
# --without-libiconv-prefix ==> don't search for libiconv in includedir and libdir

