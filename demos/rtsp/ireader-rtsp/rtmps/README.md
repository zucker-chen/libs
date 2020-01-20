
# rtmps
基于 ireader先关基础库实现的rtmp server功能  

# 实现基础
* 1，依赖https://github.com/ireader/sdk.git ， 网络及系统相关基础库实现  
* 2，依赖https://github.com/ireader/media-server.git ， 基于sdk基础库实现的相关媒体库，包括(librtp/librtsp)  
* 3，依赖libringbuf，自实现的一个ringbuf库，用来存放音视频数据缓冲  
* 4，依赖media_demuxer.c，用于ffmpeg读取h264/h265等文件的自实现库（test时才是用）  

# 已实现
* rtmp server API 接口功能  
* 支持H264/H265视频传输(FLV默认不支持H265，使用扩展给flv的videotag.codecid增加一个新类型(12)来表示h265(hevc))  
* 支持多通道视频同时链接  
* 通过valgrind内存泄露测试   


# 待实现
* 音频传输  
* rtmp client API实现  

# Tips
* VLC和完美解码默认不支持H265的RTMP播放，需要使用大牛直播客户端播放(延时更低)   


# TEST
* 基于ubuntu 64bit环境，把sdk/media-server源码clone到ireader-rtsp下  
* make
* sudo ./ ff_rtmpsrv  
* VLC打开链接(rtmp://ip:1935/live/0)播放视频  


