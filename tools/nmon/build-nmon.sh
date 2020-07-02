#!/bin/sh
# filename:         build_nmon.sh
# last modified:    2020-07-02
#
# Example usage of iconv.

enable_static_libs=true	# true
enable_cross_compile=true   # false/true
cross_prefix="arm-himix200-linux-"
ncurses_ver="ncurses-5.9"

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
	CC=${cross_prefix}gcc 
    CPP=${cross_prefix}cpp 
    CXX=${cross_prefix}g++
    CFLAGS="-D ARM"
else
	CC=gcc 
    CPP=cpp 
    CXX=g++
    CFLAGS="-D X86"
fi

CFLAGS="$CFLAGS -g -O3 -Wall -I ./${ncurses_ver}/build/include -I ./${ncurses_ver}/build/include/ncurses"
LDFLAGS="-L ./${ncurses_ver}/build/lib -lncurses -lm"

# build ncurses
if [ $enable_cross_compile = true ]; then
build_ncurses="sh build-ncurses.sh -c $cross_prefix"
else
build_ncurses="sh build-ncurses.sh"
fi
echo $build_ncurses
$build_ncurses

# build nmon
build_nmon="$CC -o nmon lmon16m.c $CFLAGS $LDFLAGS"
echo $build_nmon
$build_nmon




# Tips:
