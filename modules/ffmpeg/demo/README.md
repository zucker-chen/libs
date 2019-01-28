
# 介绍
ffmpeg转码功能用例，支持H264/H265转AVI/MP4格式

## 目的
熟悉ffmpeg转码功能基本逻辑及使用方法  
熟悉H264/H265进行AVI/MP4封装流程

## 开发使用流程
* `av_register_all`注册所需要的方法  
* `avformat_open_input/avformat_find_stream_info`根据输入文件名打开并得到输入的AVFormatContext  
* `avformat_alloc_output_context2`根据输出文件名得到输出的AVFormatContext  
* `avformat_new_stream`分配输出AVStream  
* `avcodec_find_decoder/avcodec_alloc_context3`分配AVCodecContext，并根据输入容器中的AVCodecParameters相关codec信息赋值  
* `avcodec_parameters_from_context`把前面AVCodecContext赋值给输出AVStream对应codecpar  
* `avformat_write_header/av_read_frame/av_interleaved_write_frame/av_write_trailer`封装文件读写  

## 编译
* 编译代码：`sh make.sh`  

## 执行验证
* `./build/bin/transcoding ../files/sample_cif.h264 1.avi`  
* `./build/bin/transcoding ../files/sample_cif.h264 1.mp4`  
* `./build/bin/transcoding ../files/sample_720p.h265 2.avi`  
* `./build/bin/transcoding ../files/sample_720p.h265 2.mp4`  
* `./build/bin/test_media_mux 3.mp4 ../files/sample_cif.h264 ../files/g711u_8k_2ch.wav`

## Tips
* `mp4`对`pkt.pts`有要求，要设置正确值，不然播放速率会有问题，`transcoding.c`中固定25帧视频进行处理  
* `avi`中H265对c->codec_tag有要求（由于HEVC格式很新，AVI又是比较老，tag没有匹配到）  
* `mp4`中c->codec_tag有要求，需要`=0`不然会有异常  


