#!/bin/sh
# filename:         build_boa.sh
# last modified:    2017-08-14
#
# Example usage of boa.

enable_static_libs=false	# true
enable_cross_compile=false
cross_prefix="arm-hisiv500-linux-"
target_ver="3.4.14"
output_path="$(cd `dirname $0`; pwd)/opencv-${target_ver}/output"


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
	cross_pri_cflags="-DCMAKE_C_COMPILER=${cross_prefix}gcc -DCMAKE_CXX_COMPILER=${cross_prefix}g++"
fi

# ====> ZBar Build
cd $(dirname "$0")
# Fetch Sources
if [ ! -d "opencv-${target_ver}" ]; then
    #git clone --branch 2.4.13.7 https://github.com/opencv/opencv.git
	wget https://github.com/opencv/opencv/archive/refs/tags/${target_ver}.tar.gz -O ${target_ver}.tar.gz
	tar xf ${target_ver}.tar.gz
fi
cd $(tar -tf ${target_ver}.tar.gz | awk -F "/" '{print $1}' | head -n 1)/

pri_cflags="$cross_pri_cflags
		-DCMAKE_INSTALL_PREFIX=${output_path}
		-DOPENCV_FORCE_3RDPARTY_BUILD=ON
		-DBUILD_ZLIB=ON
		-DWITH_GTK=OFF
		-DWITH_GTK=OFF
		-DWITH_GTK_2_X=OFF
		-DWITH_CUBLAS=OFF
		-DWITH_CUDA=OFF
		-DWITH_CUFFT=OFF
		-DWITH_NVCUVID=OFF
		-DWITH_IPP=OFF
		-DWITH_OPENCL=OFF
		-DWITH_OPENCLAMDBLAS=OFF
		-DWITH_QUIRC=OFF
		-DWITH_OPENCLAMDFFT=OFF
		-DWITH_1394=OFF
		-DWITH_FFMPEG=OFF
		-DWITH_WEBP=OFF
		-DWITH_TIFF=OFF
		-DWITH_OPENEXR=OFF
		-DWITH_PNG=OFF
		-DWITH_PROTOBUF=OFF
		-DWITH_GSTREAMER=OFF
		-DWITH_IMGCODEC_SUNRASTER=OFF
		-DBUILD_SHARED_LIBS=ON
		-DBUILD_opencv_ts=OFF
		-DBUILD_opencv_shape=OFF
		-DBUILD_opencv_stitching=OFF
		-DBUILD_opencv_apps=OFF
		-DBUILD_opencv_calib3d=OFF
		-DBUILD_opencv_dnn=OFF
		-DBUILD_opencv_features2d=OFF
		-DBUILD_opencv_flann=OFF
		-DBUILD_opencv_ml=OFF
		-DBUILD_opencv_objdetect=OFF
		-DBUILD_opencv_photo=OFF
		-DBUILD_opencv_video=OFF
		-DBUILD_opencv_videoio=OFF
		-DBUILD_opencv_videostab=OFF
		-DBUILD_CUDA_STUBS=OFF
		-DBUILD_opencv_cudaarithm=OFF
		-DBUILD_opencv_cudabgsegm=OFF
		-DBUILD_opencv_cudacodec=OFF
		-DBUILD_opencv_cudafeatures2d=OFF
		-DBUILD_opencv_cudafilters=OFF
		-DBUILD_opencv_cudaimgproc=OFF
		-DBUILD_opencv_cudalegacy=OFF
		-DBUILD_opencv_cudaobjdetect=OFF
		-DBUILD_opencv_cudaoptflow=OFF
		-DBUILD_opencv_cudastereo=OFF
		-DBUILD_opencv_cudawarping=OFF
		-DBUILD_opencv_cudev=OFF
		-DCMAKE_BUILD_TYPE=RELEASE"


# cmake
[ ! -d "output" ] && mkdir "output" 
[ ! -d "build" ] && mkdir -p "build"
rm -f CMakeCache.txt
cd "build"
cmake ../ $pri_cflags
# make & install
make -j8 && make install



# Tips:
# 报错："in-source builds are not allowed" ==> 保证build与output路径独立目录，然后`rm CMakeCache.txt`后重新编译

# 裁剪：
# GTK = 图形库
# CUDA = NVIDIA的CUDA显卡加速
# IPP = Intel加速的一个东西
# OPENCL = OPENCL异构计算
# QUIRC = 二维码识别
# FFMPEG = FFMPEG解码视频
# WEBP = WEBP图片格式
# TIFF = TIFF图片格式
# OPENEXR = OPENEXR图像，工业图像格式
# PNG = PNG图像格式
# PROTOBUF = PROTOBUF是DNN模块加载caffe模型的

