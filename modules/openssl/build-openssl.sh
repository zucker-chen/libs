#!/bin/sh
# filename:         build_openssl.sh
# last modified:    2017-05-13
#
# Example usage of getopts in a POSIX-compliant way.

enable_static_libs=true	# true
enable_cross_compile=false	# false
cross_prefix="arm-hisiv500-linux-"
target_ver="openssl-1.0.2l"
output_path="./build"	#"$(cd `dirname $0`; pwd)/$target_ver/build"


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

# Support full path 
cd $(dirname "$0")

# Fetch Sources
if [ ! -d $target_ver ]; then
	wget https://www.openssl.org/source/${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
cd $target_ver

# Cross compile cflags
if [ $enable_cross_compile = true ]; then
	cross_pri_cflags="no-asm shared"	# default linux-elf
fi

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path"
sh config $pri_cflags	# ;echo "sh configure $pri_cflags"

# Fix CC/AR/RANLIB in Makefile
if [ $enable_cross_compile = true ]; then
	sed -i "s/^CC=.*$/CC=${cross_prefix}gcc/" Makefile
	sed -i "s/^AR=.*$/AR=${cross_prefix}ar \$\(ARFLAGS\) r/" Makefile
	sed -i "s/^RANLIB=.*$/RANLIB=${cross_prefix}ranlib/" Makefile
	sed -i "s/-march=pentium//g" Makefile
	sed -i "s/-m64//g" Makefile
fi

make -j4 && make install


# tips:
# 源码下载地址：https://www.openssl.org/source/，github上也可以clone到源码
# 该脚本适用交叉编译openssl版本：1.0.xx 和 0.9.8x
# 交叉编译未成功openssl版本：1.1.xx ,x86编译正常
# CC/AR/RANLIB变量只能修改Makefile，通过make CC=XXX方式编译不成功，或修改环境变量 CC：export CC=arm-linux-gnueabihf-gcc
# 交叉编译和PC编译切换，需要make distclean一次
# 如果指定os/compiler 就只能用./Configure，其他情况Configure和config文件等价(如果使用./config Makefile默认添加-march=pentium)
# 报错："error: unrecognized command line option '-m64'"  ==> 将Makefile中'-m64'删除，共2处，重新编译即可  

