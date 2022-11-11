#!/bin/sh
# filename:         build_procps.sh
# last modified:    2022-11-11
#
# Build usage of procps.
#shell -e

enable_static_libs=true	# true
enable_cross_compile=false   # false/true
cross_prefix="arm-hisiv610-linux-"  # arm-himix200-linux-   arm-hisiv300-linux-
target_ver="procps-ng-4.0.1"
output_path="$(cd `dirname $0`; pwd)/$target_ver/build"
ncurses_path="$(cd `dirname $0`; pwd)/ncurses-5.9/build"

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
	cross_pri_cflags="--host=${cross_prefix%-*} CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi

cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
	wget https://udomain.dl.sourceforge.net/project/procps-ng/Production/${target_ver}.tar.xz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi

# build gdb
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path --exec-prefix=$output_path --enable-shared --enable-static --enable-static --with-ncurses"  # --disable-kill

CFLAGS="-I${ncurses_path}/include -I${ncurses_path}/include/ncurses"  \
NCURSES_CFLAGS="-I${ncurses_path}/include -I${ncurses_path}/include/ncurses" \
NCURSES_LIBS="-L${ncurses_path}/lib -lncurses" \

./configure $pri_cflags
make && make install


# Tips:
#	CFLAGS, NCURSES_CFLAGS, is needed.
#	-I${ncurses_path}/include -I${ncurses_path}/include/ncurses,  is needed.