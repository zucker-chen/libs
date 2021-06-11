#!/bin/sh
# filename:         build_python.sh
# last modified:    2021-06-10
#
# Example usage of python build.

enable_static_libs=true	# true
enable_cross_compile=true #false
cross_prefix="arm-himix100-linux-"
target_ver="Python-3.6.12"
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
                        enable_cross_compile=false
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
	cross_pri_cflags="--host=${cross_prefix%-*} --build=arm CC=${cross_prefix}gcc CPP=${cross_prefix}cpp CXX=${cross_prefix}g++"
fi
cd $(dirname "$0")
# Fetch Sources
if [ ! -f ${target_ver}.tar.xz ]; then
	wget https://www.python.org/ftp/python/${target_ver#*-}/${target_ver}.tar.xz  -O ${target_ver}.tar.xz
	tar xf ${target_ver}.tar.xz
fi

cd $(tar -tf ${target_ver}.tar.xz | awk -F "/" '{print $1}' | head -n 1)/

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path --disable-shared --disable-ipv6 ac_cv_file__dev_ptmx=no ac_cv_file__dev_ptc=no"
sh configure $pri_cflags	# ;echo "sh configure $pri_cflags"
# make & install
make -j4
make install



# Tips:
#* 报错“subprocess.CalledProcessError: Command ‘(‘lsb_release’, ‘-a’)’ returned non-zero exit status 1.”  
#	* 解决：rm -rf /usr/bin/lsb_release 后重新编译即可  
#* 交叉编译的--host应该是arm-himix100-linux， arm-linux会报错	
#* 板端运行报错“Could not find platform independent libraries <prefix>”
#	* 解决：export PYTHONHOME=$PATH
#* 板端运行报错“ModuleNotFoundError: No module named 'encodings'”
#	* 解决：由于python找不到正确的库路径，添加库路径环境变量后正常 export PYTHONPATH=/mnt/ev200/python-build/lib/python3.6  
	
	