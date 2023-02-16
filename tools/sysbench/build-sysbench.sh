#!/bin/sh
# filename:         build_procps.sh
# last modified:    2023-2-15
#
# Build usage of procps.
#shell -e

enable_static_libs=true	# true
enable_cross_compile=true   # false/true
cross_prefix="aarch64-linux-gnu-"  # arm-himix200-linux-   arm-hisiv300-linux-
target_ver="1.0.20"
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
                        enable_cross_compile=true
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
	cross_pri_cflags="--host=${cross_prefix%-*} CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++ PLATFORM=aarch64"
fi

cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
	wget https://github.com/akopytov/sysbench/archive/refs/tags/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi

# build gdb
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path --exec-prefix=$output_path --disable-shared --enable-static --without-mysql"  # --disable-kill --enable-shared

# sudo apt-get install automake
# sudo apt-get install libtool
#./autogen.sh
./configure $pri_cflags

# 

make && make install


# Tips:
#	1, concurrency_kit编译arm时需要制定平台，默认不会交叉编译，是X86
#      sysbench-1.0.20/third_party/concurrency_kit/Makefile.in，CC下面添加"PLATFORM="aarch64"					\"
#	2，luajit编译arm也需要指定平台编译器
#      /home/zucker/Project/8.debug/libs/tools/sysbench/sysbench-1.0.20/third_party/luajit，PREFIX上面添加"HOST_CC="gcc" CROSS=aarch64-linux-gnu-	\"
#      
