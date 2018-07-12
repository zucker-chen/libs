#!/bin/sh
# filename:         build_SDL.sh
# last modified:    2018-07-11
#
# Example usage of SDL.

enable_static_libs=false	# true
enable_cross_compile=false
cross_prefix="arm-hisiv500-linux-"
target_ver="sdl-2.0"
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
if [ ! -f ${target_ver}.tar.gz ]; then
	wget https://hg.libsdl.org/SDL/archive/tip.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
output_path="$(pwd)/build"    # 重新指定后，output_path输入传参将失效

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path 
--enable-audio=no --enable-pulseaudio=no --enable-pulseaudio-shared=no --enable-arts=no --enable-nas=no --enable-sndio=no --enable-fusionsound=no --enable-diskaudio=no --enable-libsamplerate=no
--enable-joystick=no --enable-esd=no --enable-input-tslib=no
--enable-video-mir=no --enable-video-rpi=no --enable-video-x11=no --enable-video-x11-xcursor=no --enable-video-x11-xdbe=no --enable-video-x11-xinerama=no --enable-video-x11-xinput=no --enable-video-x11-xrandr=no --enable-video-x11-scrnsaver=no --enable-video-x11-xshape=no --enable-video-x11-vm=no --enable-video-vivante=no --enable-video-cocoa=no --enable-video-directfb=no --enable-video-kmsdrm=no --enable-video-opengl=no --enable-video-opengles=no --enable-video-opengles1=no --enable-video-opengles2=no --enable-video-vulkan=no
"   # 裁剪只节省67KB(striped)

sh configure $pri_cflags	# ;echo "sh configure $pri_cflags" && return
# make & install
make -j4 && make install



# Tips:


