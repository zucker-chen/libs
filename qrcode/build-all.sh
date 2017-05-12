#!/bin/sh
# filename:         build-all.sh
# last modified:    2017-05-13
#
# Example usage of getopts in a POSIX-compliant way.

enable_static_libs=false	# true
enable_cross_compile=false
cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
output_path="$(cd `dirname $0`; pwd)/build"


usage()
{
    printf "Usage: %s [-c [cross_prefix]] [-o [output_path]] [-hs]\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    -c <cross_prefix>  	enable cross compile, with cross_prefix\n"
    printf "    -o <output_path>	output_path is build out path\n"
    printf "    -s 			enable static\n"
    printf "    -h  		print usage and exit\n"
}

fun_getopts()
{
    while getopts :c:l:o::hs option; do
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
	printf "output_path=%s\n" "${output_path}"	
}

fun_getopts "$@"


zbar_ver="zbar-0.10"
libiconv_ver="libiconv-1.14"
imagemagick_ver="ImageMagick-6.9.8-4"

if [ "$enable_cross_compile" = true ]; then
# Build zbar
	sh $(dirname "$0")/build-zbar.sh -c $cross_prefix -t $zbar_ver -s
	# Build libiconv
	sh $(dirname "$0")/build-libiconv.sh -c $cross_prefix -t $libiconv_ver -s
	# Build imagemagick
	sh $(dirname "$0")/build-imagemagick.sh -c $cross_prefix -t $imagemagick_ver -s
else
	# Build zbar
	sh $(dirname "$0")/build-zbar.sh -t $zbar_ver -s
	# Build libiconv
	sh $(dirname "$0")/build-libiconv.sh -t $libiconv_ver -s
	# Build imagemagick
	sh $(dirname "$0")/build-imagemagick.sh -t $imagemagick_ver -s
fi

# Copy zbar libs and includes
[ ! -d $output_path/lib ] && mkdir -p $output_path/lib
[ ! -d $output_path/include ] && mkdir -p $output_path/include
cp -a $(dirname "$0")/$zbar_ver/build/lib/libzbar.so* $output_path/lib/
cp -a $(dirname "$0")/$zbar_ver/build/include/zbar.h $output_path/include/
cp -a $(dirname "$0")/$libiconv_ver/build/lib/libiconv.so* $output_path/lib/
cp -a $(dirname "$0")/$libiconv_ver/build/include/iconv.h  $output_path/include/
cp -a $(dirname "$0")/$imagemagick_ver/build/lib/libMagickWand-6.Q16.so*  $output_path/lib/
cp -a $(dirname "$0")/$imagemagick_ver/build/include/ImageMagick-6/* $output_path/include/


