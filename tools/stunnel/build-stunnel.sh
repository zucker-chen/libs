#!/bin/sh
# filename:         build_stunnel.sh
# last modified:    2021-11-11
#
# Example usage of stunnel.

enable_static_libs=true	# true
enable_cross_compile=true
cross_prefix="arm-hisiv500-linux-"
target_ver="stunnel-5.60"
openssl_path="$(cd `dirname $0`; pwd)/openssl-1.1.1c/build"
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
if [ ! -f ${target_ver}.tar.xz ]; then
	wget https://github.com/mtrojnar/stunnel/archive/refs/tags/${target_ver}.tar.gz -O ${target_ver}.tar.xz
	tar xf ${target_ver}.tar.xz
fi
target_dir=$(tar -tf ${target_ver}.tar.xz | awk -F "/" '{print $1}' | head -n 1)
output_path=$(pwd)/$target_dir/build
cd $target_dir

mkdir -p $output_path/sbin $output_path/bin $output_path/share


# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-shared --enable-static --with-ssl=$openssl_path"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
make install



# Tips:
# #error OpenSSL library compiled without thread support  ===> 编译openssl时不能用  no-threads 选项
# ssl的库可以是动态库，也可以是静态库，看项目需求，交叉编译时默认静态库编译
# 静态库交叉编译ssl，stunnel执行文件strip后大小为2.3MB