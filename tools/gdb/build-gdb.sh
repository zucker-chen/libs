#!/bin/sh
# filename:         build_gdb.sh
# last modified:    2021-10-16
#
# Example usage of iconv.
shell -e

enable_static_libs=true	# true
enable_cross_compile=false   # false/true
cross_prefix="arm-himix200-linux-"  # arm-himix200-linux-   arm-hisiv300-linux-
target_ver="gdb-7.10"
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
else
    cross_pri_cflags="--target=${cross_prefix%-*}"
fi

cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.gz ]; then
    # gdb
    wget http://ftp.gnu.org/gnu/gdb/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi

# build gdb
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
pri_cflags="$cross_pri_cflags --prefix=$output_path --enable-shared --enable-static LDFLAGS=-L${ncurses_path}/lib LIBS=-lncurses --disable-tui"  
./configure --disable-werror $pri_cflags	# ;echo "sh configure $pri_cflags"
# Remove the keyword const.
sed -i "s/ps_get_thread_area (const struct ps_prochandle/ps_get_thread_area (struct ps_prochandle/" gdb/arm-linux-nat.c 
sed -i "s/ps_get_thread_area (const struct ps_prochandle/ps_get_thread_area (struct ps_prochandle/" gdb/gdbserver/linux-arm-low.c
# make
gdb_cv_prfpregset_t_broken=no make -j4 && make install


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
#   missing: makeinfo: not found   ==> sudo apt-get install makeinfo，或 sudo apt-get install texinfo，如果仍失败需要重新解压源码编译就OK
#   交叉编译失败：
#   ui-file.h:43:18: error: macro "putc"   ==> 未解决  
# 6, _24273.c:843:15: error: expected ‘)’ before ‘int’ ==> export CPPFLAGS="-P"
# 7, linux 64位编译32位程序(即-m32支持)方法  ==> sudo apt install libc6-dev-i386 g++-multilib
# 8, 7.10版本交叉编译问题：  
#    8.1 linux-arm-low.c:337:1: error: conflicting types for ‘ps_get_thread_area’  ==> ps_get_thread_area (const struct ps_prochandle *ph, 把const关键字去掉即可，最新版本验证又正常(不需要特殊处理)  
#    8.2 gdb_proc_service.h:162:9: error: unknown type name ‘gdb_fpregset_t’
#        proc-service.c:197:1: error: conflicting types for ‘ps_lgetfpregs’        ==> make前面加上“gdb_cv_prfpregset_t_broken=no”  
# 9, 如果gdb需要远程调试(+gdbserver)的话，gdb编译需要指定远程目标机环境`--target=${cross_prefix%-*}`，该gdb二进制就只能解析板端的执行程序，编译出来的gdb名字是`arm-himix200-linux-gdb`                  
# 10, configure: error: `target_alias' has changed since the previous run:   ==> 重新删除源码后解压新的编译   
