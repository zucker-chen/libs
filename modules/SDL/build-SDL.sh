#!/bin/sh
# filename:         build_SDL.sh
# last modified:    2018-07-11
#
# Example usage of SDL.

enable_static_libs=true	# true
enable_cross_compile=false
cross_prefix="arm-hisiv300-linux-"
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
	#wget https://hg.libsdl.org/SDL/archive/tip.tar.gz -O ${target_ver}.tar.gz              # 最新版本
    #wget https://hg.libsdl.org/SDL/archive/fba40d9f4a73.tar.gz -O ${target_ver}.tar.gz     # release-1.2.15
    wget https://hg.libsdl.org/SDL/archive/f1084c419f33.tar.gz  -O ${target_ver}.tar.gz     # release-2.0.8
	tar xf ${target_ver}.tar.gz
fi
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/
output_path="$(pwd)/build"    # 重新指定后，output_path输入传参将失效

# ./configure
pri_cflags="$cross_pri_cflags --prefix=$output_path 
--disable-dependency-tracking --disable-audio --disable-video --disable-render --disable-events --disable-joystick --disable-haptic --disable-power --disable-filesystem --disable-timers
--disable-loadso --disable-cpuinfo --disable-mmx --disable-3dnow --disable-sse --disable-sse2 --disable-sse3 --disable-altivec --disable-oss 
--disable-alsa --disable-alsatest --disable-alsa-shared --disable-jack --disable-jack-shared --disable-esd --disable-esdtest --disable-esd-shared --disable-pulseaudio 
--disable-pulseaudio-shared --disable-arts --disable-arts-shared --disable-nas --disable-nas-shared --disable-sndio --disable-sndio-shared --disable-fusionsound --disable-fusionsound-shared 
--disable-diskaudio --disable-dummyaudio --disable-libsamplerate --disable-libsamplerate-shared --disable-video-wayland --disable-video-wayland-qt-touch --disable-wayland-shared --disable-video-mir --disable-mir-shared 
--disable-video-rpi --disable-video-x11 --disable-x11-shared --disable-video-x11-xcursor --disable-video-x11-xdbe --disable-video-x11-xinerama --disable-video-x11-xinput --disable-video-x11-xrandr --disable-video-x11-scrnsaver 
--disable-video-x11-xshape --disable-video-x11-vm --disable-video-vivante --disable-video-cocoa --disable-render-metal --disable-video-directfb --disable-directfb-shared --disable-video-kmsdrm --disable-kmsdrm-shared 
--disable-video-dummy --disable-video-opengl --disable-video-opengles --disable-video-opengles1 --disable-video-opengles2 --disable-video-vulkan --disable-libudev --disable-dbus --disable-ime 
--disable-ibus --disable-fcitx --disable-input-tslib --disable-directx --disable-wasapi --disable-render-d3d
"   # 通过参数裁剪大小有限(1M左右)，主要靠strip

# strip libs 
#strip --strip-debug --strip-unneeded libSDL2.a

sh configure $pri_cflags	# ;echo "sh configure $pri_cflags" && return
# make & install
make -j4 && make install



# Tips:
# 1 静态库strip需要加参数，不然会有异常： #strip --strip-debug --strip-unneeded libSDL2.a

