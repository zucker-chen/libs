#!/bin/sh
# filename:         build_ncurses.sh
# last modified:    2021-10-16
#
# Example usage of iconv.

enable_static_libs=true	# true
enable_cross_compile=false   # false/true
cross_prefix="arm-hisiv300-linux-"
target_ver="ncurses-5.9"
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
	cross_pri_cflags="--host=${cross_prefix%-*} CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
else
    cross_pri_cflags="--target=${cross_prefix%-*}"
fi

cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
    # ncurses
    wget http://ftp.gnu.org/gnu/ncurses/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
	cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
	wget -O config.guess 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
	wget -O config.sub 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
	cd -
fi

# build ncurses
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
export CPPFLAGS="-P"   # -m32"
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-shared --enable-static"
sh configure --disable-werror $pri_cflags	# ;echo "sh configure $pri_cflags"
make -j4 && make install
cd -


# Tips:
# 7.10与7.6.2版本可正常编译使用，但是gdb+gdbserver调试在32/64位交叉环境下不行
# 1, bfd.h:529:65: error: right-hand operand of comma expression has no effect [-Werror=unused-value]   ==> --disable-werror
# 2, 7.6.2版本，PC编译OK，交叉编译报错解决：
#   configure: error: `host_alias' has changed since the previous run
#   error: `LDFLAGS' has changed since the previous run     ==> 重新解压源码包再编译，make distclean都不行
#   configure: error: no termcap library found ==> install ncurses lib
#   gdb+gdbserver调试时：Remote 'g' packet reply is too long:    ==> 原因：由于目标机是32位系统，PC是64位系统，架构不一致导致；需要修改gdb源码remote.c [参考README.md]
#   -m32编译32位gdb时报错："amd64-linux-nat.c:53:3: error: ‘RAX’ undeclared here (not in a function)" 未解决
# 3, 7.10与7.6.2版本编译情况一样，gdb+gdbserver调试时会出现异常
# 4, 7.12版本交叉编译失败(undefined reference to `_obstack_free')，PC编译OK
# 5, 8.2版本PC编译失败(collect2: error: ld returned 1 exit status)
#   missing: makeinfo: not found   ==> sudo apt-get install makeinfo，如果仍失败需要重新解压源码编译就OK
#   交叉编译失败：
#   ui-file.h:43:18: error: macro "putc"   ==> 未解决  
# 6, _24273.c:843:15: error: expected ‘)’ before ‘int’ ==> export CPPFLAGS="-P"
# 7, linux 64位编译32位程序(即-m32支持)方法  ==> sudo apt install libc6-dev-i386 g++-multilib
# 8, build ncurses error: 1.sh  2.sh  build-gdb.sh  build-ncurses.sh  ncurses-5.9  ncurses-5.9.tar.gz  README.md
#    wget -O config.guess 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
#    wget -O config.sub 'http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'

