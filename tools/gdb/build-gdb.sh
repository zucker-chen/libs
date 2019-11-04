#!/bin/sh
# filename:         build_iconv.sh
# last modified:    2019-08-16
#
# Example usage of iconv.

enable_static_libs=true	# true
enable_cross_compile=false   # false
cross_prefix="arm-hisiv300-linux-"
target_ver="gdb-7.10"
target_ver2="ncurses-5.9"
output_path="$(cd `dirname $0`; pwd)/$target_ver/build"
output_path2="$(cd `dirname $0`; pwd)/$target_ver2/build"

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
	cross_pri_cflags="--host=arm-hisiv300-linux CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi
cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
    # gdb
    wget http://ftp.gnu.org/gnu/gdb/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
    # ncurses
    #wget http://ftp.gnu.org/gnu/ncurses/${target_ver2}.tar.gz -O ${target_ver2}.tar.gz
	#tar xf ${target_ver2}.tar.gz
fi

# build ncurses
cd $(tar -tf ${target_ver2}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path2 --enable-shared --enable-static"
#sh configure --disable-werror $pri_cflags	# ;echo "sh configure $pri_cflags"
#make -j4 && make install
cd -

# build gdb
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-shared --enable-static LDFLAGS=-L${output_path2}/lib LIBS=-lncurses --disable-tui"
./configure --disable-werror $pri_cflags	# ;echo "sh configure $pri_cflags"
make -j4 && make install


# Tips:
# 1, bfd.h:529:65: error: right-hand operand of comma expression has no effect [-Werror=unused-value]   ==> --disable-werror
# 2, 7.6.2版本，PC编译OK，交叉编译报错解决：
#   configure: error: `host_alias' has changed since the previous run
#   error: `LDFLAGS' has changed since the previous run     ==> 重新解压源码包再编译，make distclean都不行
#   configure: error: no termcap library found ==> install ncurses lib
#   gdb+gdbserver调试时：Remote 'g' packet reply is too long:    ==> 需要修改gdb源码remote.c，自行百度
# 3, 7.10与7.6.2版本编译情况一样，gdb+gdbserver调试时会出现异常
# 4, 7.12版本交叉编译失败(undefined reference to `_obstack_free')，PC编译OK
# 5, 8.2版本PC编译失败(collect2: error: ld returned 1 exit status)
#   missing: makeinfo: not found   ==> sudo apt-get install makeinfo，如果仍失败需要重新解压源码编译就OK
#   交叉编译失败：
#   ui-file.h:43:18: error: macro "putc"   ==> 未解决  

