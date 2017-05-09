#!/bin/sh

enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# ====> ImageMagick Build
imagemagick_ver="ImageMagick-7.0.5-5"
output_path="$(pwd)/$imagemagick_ver/build"
# Fetch Sources
if [ ! -d $imagemagick_ver ]; then
	#wget http://downloads.jmagick.org/6.4.0/${imagemagick_ver}.tar.gz
	wget https://www.imagemagick.org/download/${imagemagick_ver}.tar.bz2
	tar xf ${imagemagick_ver}.tar.bz2
fi
cd $imagemagick_ver
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static
			--without-magick-plus-plus --without-bzlib --without-x --without-zlib --without-dps --disable-docs"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install
cd -

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

# ====> ZBar Build
zbar_ver="zbar-0.10"
output_path="$(pwd)/$zbar_ver/build"
# Fetch Sources
if [ ! -d $zbar_ver ]; then
	wget http://downloads.sourceforge.net/project/zbar/zbar/0.10/zbar-0.10.tar.bz2
	tar xf ${zbar_ver}.tar.bz2
fi
cd $zbar_ver
# "./libtool: eval: line 961: syntax error near unexpected token `|'"
export NM=nm
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$zbar_output_path --enable-static
			--without-gtk --without-qt --without-python --disable-video
			--without-jpeg --without-xv --without-xshm --without-x
			CPPFLAGS=-I$output_path/include LDFLAGS=-L$output_path/lib"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install



# Note:
# libiconv must profile ==> "zbar/qrcode/qrdectxt.c:9:19: error: iconv.h: No such file or directory"
# libiconv-1.15 ==> "getprogname.c:147:4: error: #error "getprogname module not ported to this OS""
# imagemagick ==> used for read image(gray raw data) which ready for zbar scan.
# Magick++ ==> C++ API
