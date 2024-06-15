#!/bin/sh
# filename:         build_util-linux.sh
# last modified:    2024-06-09
#
# Example usage of util-linux.

enable_static_libs=true	# true
enable_cross_compile=true #false/true
cross_prefix="arm-hisiv500-linux-"
target_ver="v2.40.1"
output_path="$(cd `dirname $0`; pwd)/util-linux-2.40.1/build"


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
if [ ! -f ${target_ver}.tar.gz ]; then
	wget https://codeload.github.com/util-linux/util-linux/tar.gz/refs/tags/${target_ver}  -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
echo $(pwd)


# check configure
if [ ! -f configure ]; then
	./autogen.sh
fi

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --disable-year2038 --disable-liblastlog2 --disable-lsfd --disable-pipesz --disable-unshare --disable-exch --disable-libsmartcols"
if [ $enable_static_libs = true ]; then
	pri_cflags="$pri_cflags --enable-static --disable-shared"
fi


sh configure -v $pri_cflags CFLAGS="-std=c11" CXXFLAGS="-std=c++11" LDFLAGS="-lrt -lc"	# ;echo "sh configure $pri_cflags" && exit 0
# replace O_PATH.    --> context_umount.c:294:18: error: ‘O_PATH’ undeclared
sed -i "s/fd = open(tgt, O_PATH);/fd = open(tgt, O_RDONLY | O_DIRECTORY);/" libmount/src/context_umount.c
sed -i "s/fd = open(tgt, O_PATH);/fd = open(tgt, O_RDONLY | O_DIRECTORY);/" sys-utils/fstrim.c
# replace setns.    --> undefined reference to `setns'
sed -i "s/sched\.h/sys\/syscall\.h/" libmount/src/hook_subdir.c
sed -i "s/setns(/syscall(SYS_setns, /" libmount/src/hook_subdir.c
# replace static_assert.    --> undefined reference to `static_assert'
sed -i "s/static_assert(/_Static_assert(/" include/xxhash.h
# add #include <elf.h>.    --> audit-arch.h:17:33: error: ‘EM_ARM’ undeclared 
sed -i 's/^#include <sys\/ioctl.h>/#include <sys\/ioctl.h>\n#include <elf.h>/' misc-utils/enosys.c
# add #include <elf.h>.    --> audit-arch.h:17:33: error: ‘EM_ARM’ undeclared 
sed -i 's/^#include <stdio.h>/#include <stdio.h>\n#include <unistd.h>\n#include <errno.h>/' misc-utils/lsfd.c
# del ‘PER_LINUX32_3GB’.    --> setarch.c:106:7: error: ‘PER_LINUX32_3GB’ undeclared here 
sed -i '/X(PER_LINUX32_3GB)/'d sys-utils/setarch.c


# make & install
make -j4
make install



# Tips:
# 1. libmount.so: undefined reference to `static_assert'
#   ./configure --enable-static --disable-shared
# 2. undefined reference to `xxx'
#   --disable-libblkid ....
#   make distclean && rm configure
# 3. undefined reference to `clock_getcpuclockid'
#   ./configure LDFLAGS="-lrt"
# 4. audit-arch.h:17:33: error: ‘EM_ARM’ undeclared 
#   #include <elf.h>
# 4. setarch.c:106:7: error: ‘PER_LINUX32_3GB’ undeclared here
#   delete this line
# 5. configure: WARNING: libpython not found; 
#   export PYTHON_CONFIG=python3-config
#   ./configure
# 6. configure: WARNING: libblkid disabled; not building libmount
# 7. misc-utils/lsfd.c:66:2: error: ‘errno’ undeclared (first use in this function)
#   #include <errno.h>
# 8. misc-utils/lsfd.c:1661:7: error: ‘__NR_select’ undeclared
#   #include <sys/syscall.h>
#   // 或者
#   #include <unistd.h>
# 9. undefined reference to `static_assert'
#   replace by _Static_assert
# 10. undefined reference to `setns'
#   replace by syscall(SYS_setns, fd, nstype)