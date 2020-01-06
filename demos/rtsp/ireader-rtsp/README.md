
# ireader-rtsp
基于 ireader先关基础库实现的rtsp功能  

# 实现基础
* 1，依赖https://github.com/ireader/sdk.git ， 网络及系统相关基础库实现  
* 2，依赖https://github.com/ireader/media-server.git ， 基于sdk基础库实现的相关媒体库，包括(librtp/librtsp)  
* 3，依赖libringbuf，自实现的一个ringbuf库，用来存放音视频数据缓冲  
* 4，依赖media_demuxer.c，用于ffmpeg读取h264/h265等文件的自实现库（test时才是用）  

# 已实现
* rtsp server API 接口功能  
* 支持RTP for TCP/UDP方式  
* 支持H264/H264视频传输  
* 支持多通道视频同时链接  
* 使用线程池，解决多用户连接点流出现卡顿问题  
* 支持rtsp server option/describe 方法认证，同时支持空用户名、空密码  
* 通过valgrind内存泄露测试   


# 待实现
* 音频传输  
* rtsp client API实现  

# Tips
* RTSP所谓的TCP/UDP连接方式，是指RTP音视频数据传输的方式，RTSP信令走HTTP方式及TCP通信，目前没有找到UDP方式client测试   


# TEST
* 基于ubuntu 64bit环境，把sdk/media-server源码clone到ireader-rtsp下  
* make
* sudo ./ ff_rtsps_srv  
* VLC打开链接(rtsp://ip:554/live/0)播放视频


