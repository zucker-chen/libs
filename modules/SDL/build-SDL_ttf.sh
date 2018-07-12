#!/bin/sh
# filename:         build_SDL_ttf.sh
# last modified:    2018-07-11
#
# Example usage of SDL_ttf.

enable_static_libs=false	# true
enable_cross_compile=false
cross_prefix="arm-hisiv500-linux-"
target_ver="sdl_ttf-2.0"
output_path="$(cd `dirname $0`; pwd)/$target_ver/build"


usage()
{
    printf "Usage: %s [-c [cross_prefix]] [-v [target_ver]] [-o [output_path]] [-hs]\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    -c <cross_prefix>  	enable cross compile, with cross_prefix\n"
    printf "    -v <target_ver>		target_ver is lib version\n"
    printf "    -o <output_path>	output_path is build out path\n"
    printf "    -s 			enable static\n"
    printf "    -h  		print usage and exit\n"
}

fun_getopts()
{
    while getopts :c:v:o::hs option; do
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

            v)
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
if [ $enable_cross_compile = true ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi
cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
	wget https://hg.libsdl.org/SDL_ttf/archive/tip.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
output_path="$(pwd)/build"    # 重新指定后，output_path输入传参将失效

# ./configure
export PKG_CONFIG_PATH=$(pwd)/../freetype2-VER-2-8/build/lib/pkgconfig:$(pwd)/../SDL-1a1133e9c7d4/build/lib/pkgconfig
pri_cflags="$cross_pri_cflags --prefix=$output_path --with-freetype-prefix=$(pwd)/../freetype2-VER-2-8/build --with-sdl-prefix=$(pwd)/../SDL-1a1133e9c7d4/build"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4 && make install



# Tips:
# 需要借助pkg-config工具添加freetype2库路径，如果不export PKG_CONFIG_PATH环境变量会报错："fatal error: ft2build.h: No such file or directory"
# 指定SDL/freetype2库路径 --with-freetype-prefix, --with-sdl-prefix
