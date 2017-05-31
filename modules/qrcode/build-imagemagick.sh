#!/bin/sh
# filename:         build_imagemagick.sh
# last modified:    2017-05-13
#
# Example usage of getopts in a POSIX-compliant way.

enable_static_libs=false	# true
enable_cross_compile=false
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
target_ver="ImageMagick-6.9.8-4"
output_path="$(cd `dirname $0`; pwd)/$target_ver/build"


usage()
{
    printf "Usage: %s [-c [cross_prefix]] [-t [target_ver]] [-o [output_path]] [-hs]\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    -c <cross_prefix>  	enable cross compile, with cross_prefix\n"
    printf "    -t <target_ver>		target_ver is lib version\n"
    printf "    -o <output_path>	output_path is build out path\n"
    printf "    -s 			enable static\n"
    printf "    -h  		print usage and exit\n"
}

fun_getopts()
{
    while getopts :c:t:o::hs option; do
        case "${option}" in
            c)
				case "${OPTARG}" in
                    "")
                        enable_cross_compile=true
                        ;;
                    *)
                        cross_prefix="${OPTARG}"
                        ;;
                esac
                ;;

            t)
                target_ver="${OPTARG}"
                ;;

            o)
                output_path="${OPTARG}"
                ;;

            h)
                usage
                exit 0
                ;;

            s)
                enable_static_libs=true
                ;;

            --)
                ;;

            \?)
                printf "Error: Invalid option: -%s\n" "${OPTARG}" >&2
                usage
                exit 1
                ;;

            :)
                printf "Error: Option -%s requires an argument\n" "${OPTARG}" >&2
                usage
                exit 1
                ;;
        esac
    done

    shift $((OPTIND - 1))

	printf "enable_static_libs=%s\n" "${enable_static_libs}"	
	printf "enable_cross_compile=%s\n" "${enable_cross_compile}"	
	printf "cross_prefix=%s\n" "${cross_prefix}"	
	printf "target_ver=%s\n" "${target_ver}"	
	printf "output_path=%s\n" "${output_path}"	
}

fun_getopts "$@"



# Cross compile cflags
if [ "$enable_cross_compile" = true ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

# ====> ImageMagick Build
cd $(dirname "$0")
# Fetch Sources
if [ ! -d $target_ver ]; then
	wget https://www.imagemagick.org/download/${target_ver}.tar.xz
	tar xf ${target_ver}.tar.xz
fi
cd $target_ver
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