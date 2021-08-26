#!/bin/sh
# filename:         build_pcre.sh
# last modified:    2021-08-26
#
# Example usage of nginx.
shell -e

enable_static_libs=true	# true
enable_cross_compile=true   # false/true
cross_prefix="arm-himix100-linux-"  # arm-himix200-linux-   arm-hisiv300-linux-
target_ver="nginx-1.20.1"
output_path="$(cd `dirname $0`; pwd)/$target_ver/build"
pcre_path="$(cd `dirname $0`; pwd)/pcre-8.44/build"
openssl_path="$(cd `dirname $0`; pwd)/openssl-1.1.1c/build"
nginx_patch_path="$(cd `dirname $0`; pwd)/nginx-patch"
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
	#cross_pri_cflags="CC=${cross_prefix}gcc CPP=${cross_prefix}g++ CXX=${cross_prefix}g++"
	export CC=${cross_prefix}gcc
	export CPP=${cross_prefix}g++
fi

cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
	wget http://nginx.org/download/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi

# build gdb
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path --builddir=$output_path
			--without-http_gzip_module --with-http_ssl_module --with-pcre=$pcre_path
			--with-openssl=$openssl_path --with-cc=$CC --with-cpp=$CPP
			--without-http_upstream_zone_module
			--with-ld-opt=-lpthread"  

# configure前需要修改相关配置文件
cp -r $nginx_patch_path/$target_ver/* $output_path/../

./configure $pri_cflags	# ;echo "sh configure $pri_cflags"

# configure完需要修改自动生成的ngx_auto_config.h文件
cp -r $nginx_patch_path/$target_ver/* $output_path/../


# make
make -j4 && make install


# Tips:
# 报错："error: C compiler arm-himix100-linux-gcc is not found" ==> 修改auto/cc/name， ngx_feature_run=no
# 报错："error: can not detect int size" ==> 修改auto/types/sizeof，ngx_test中的 $CC 为 gcc 和 ngx_size=`$NGX_AUTOTEST` 为  ngx_size=4
# 报错："#error ngx_atomic_cmp_set() is not defined!" ==> 在configure的时候加上--without-http_upstream_zone_module
# 报错：" undefined reference to 'ngx_shm_alloc'" ==> 修改 build/ngx_auto_config.h，添加 #ifndef NGX_HAVE_SYSVSHM #define NGX_HAVE_SYSVSHM  1 #endif
# 报错："undefined reference to `pthread_atfork'" ==> 由于openssl依赖pthread，可以编译openssl 添加no-threads选项 


