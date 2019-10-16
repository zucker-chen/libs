#!/bin/sh
# filename:         build_openssl.sh
# last modified:    2017-05-13
#
# Example usage of getopts in a POSIX-compliant way.

enable_static_libs=true	# true
enable_cross_compile=false	# false
cross_prefix="arm-hisiv500-linux-"
target_ver="jpegsrc.v9c"
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
if [ ! -f ${target_ver}.tar.gz ]; then
    wget http://www.ijg.org/files/${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
#cd $target_ver
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
output_path=$(cd `dirname $0`; pwd)/$output_path

# Cross compile cflags
if [ $enable_cross_compile = true ]; then
	cross_pri_cflags="--host=arm-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"	# default linux-elf
fi

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-shared --enable-static"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"


make -j4 && make install


# Tips:
# 1, configure: error: expected an absolute directory name for --prefix: ./build



