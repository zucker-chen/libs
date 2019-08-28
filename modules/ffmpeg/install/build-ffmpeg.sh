#!/bin/sh
enable_cross_compile="disable"	# enable/disable

enable_shared_libs="disable"	# enable/disable
cross_prefix="arm-hisiv300-linux-"
output_path="./build"
enable_x264="disable"	# enable/disable
ffmpeg_ver="ffmpeg-3.3"

# Fetch Sources
if [ ! -d $ffmpeg_ver ]; then
	wget http://ffmpeg.org/releases/${ffmpeg_ver}.tar.bz2
	tar xf ${ffmpeg_ver}.tar.bz2
fi

# x264
if [ "$enable_x264" = "enable" ]; then
	sh build-x264.sh $enable_shared_libs $enable_cross_compile $cross_prefix
	x264_lib_path="../../x264/build/lib"
	x264_inc_path="../../x264/build/include"
	extra_lib_cflags="--enable-libx264 --enable-gpl --extra-cflags=-I$x264_inc_path --extra-ldflags=-L$x264_lib_path"
fi

# shared libs, default static
if [ "$enable_shared_libs" = "enable" ]; then
	shared_libs_cflags="--enable-shared --disable-static"
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="--cross-prefix=$cross_prefix --enable-cross-compile --target-os=linux --target-os=linux  --arch=arm"
fi

# ./configure
pri_cflags="$cross_pri_cflags
			--prefix=$output_path --disable-yasm
			--disable-ffplay --disable-ffprobe  --disable-ffserver 
			$shared_libs_cflags
			$extra_lib_cflags
			--disable-everything 
			--enable-protocol=file 
			--enable-decoder=h264 --enable-decoder=hevc --enable-decoder=aac --enable-decoder=pcm_alaw --enable-decoder=pcm_mulaw
			--enable-encoder=aac --enable-encoder=pcm_alaw --enable-encoder=pcm_mulaw
			--enable-demuxer=avi --enable-demuxer=mov --enable-demuxer=h264 --enable-demuxer=hevc --enable-demuxer=aac --enable-demuxer=wav --enable-demuxer=pcm_alaw --enable-demuxer=pcm_mulaw --enable-demuxer=matroska
			--enable-muxer=avi --enable-muxer=mp4 --enable-muxer=pcm_alaw --enable-muxer=pcm_mulaw --enable-muxer=matroska
			--enable-parser=h264 --enable-parser=hevc
			--enable-small --disable-debug --disable-doc
			--disable-avdevice --disable-swscale --disable-postproc"

echo "sh configure $pri_cflags"
cd $ffmpeg_ver && sh configure $pri_cflags

# make & install
make -j4 && make install

[ ! $? = 0 ] && echo "#### try: make -C $ffmpeg_ver distclean"

echo "#### make install success. output path = $ffmpeg_ver/build"

#
# test cmd: "./ffmpeg -i test.h264 output.avi" "./ffmpeg -i test.h265 output.avi"
# --enable-demuxer=h264 --enable-demuxer=hevc used for "Invalid data found when processing input"
# --enable-parser=h264 --enable-parser=hevc, used for input file parser.
# --enable-muxer=avi --enable-encoder=mpeg4, need for avi encoding.
# --enable-protocol=file , if add it, "./ffmpeg -i test.h264 output.avi" will be abnormal(Protocol not found).
# --disable-avfilter --disable-swresample, if add it, ffmpeg bin cannot build.
# if ./ffmpeg -version ==> "libavfilter.so.6: cannot open shared object file", try: make -C $ffmpeg_ver distclean.
#
# libx264: used for encoding h264 streams, default disable it.
# hevc_ps.c:(.text+0xc20): undefined reference to `ff_hevc_diag_scan4x4_y' and undefined reference to `ff_reverse'   ===> make distclean before make
# WARNING: Option --enable-demuxer=mp4 did not match anything  ==> just --enable-muxer=mp4//mp4 and mov have the same demuxer, so use --enable-demuxer=mov ?
# mov.c:(.text+0x31a5): undefined reference to `uncompress' ==> --enable-demuxer=mov
# Could not find codec parameters for stream 0 (Video: h264, none(progressive)): unspecified size ==> --enable-decoder=h264 --enable-decoder=hevc
# Could not open input file 'g711u_8k.wav', Invalid data found when processing input ==> --enable-demuxer=wav
# mov.c:(.text+0x31a5): undefined reference to `uncompress'  ===> -lz
# .mkv support: --enable-muxer=matroska --enable-demuxer=matroska
