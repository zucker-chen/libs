#!/bin/sh

enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# ====> ImageMagick Build
imagemagick_ver="ImageMagick-6.9.8-4"
output_path="$(pwd)/$imagemagick_ver/build"
# Fetch Sources
if [ ! -d $imagemagick_ver ]; then
	wget https://www.imagemagick.org/download/${imagemagick_ver}.tar.xz
	tar xf ${imagemagick_ver}.tar.xz
fi
cd $imagemagick_ver
# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-static
			--without-magick-plus-plus --without-bzlib --without-x --without-zlib --without-dps --disable-docs"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install
cd -



# Note:
# imagemagick ==> used for read image(gray raw data) which ready for zbar scan.
# ImageMagick-7.0.5-5 ==> had some problem used for zbar
# MagickCore ==> 底层的C语言接口。较复杂，但是可以修改很多参数，只适合高端用户使用
# MagickWand ==> 推荐的C语言接口。相比于MagickCore接口，简单很多。适合普通用户使用
# Magick++ ==> C++ APIS