
# ireader-rtsp
基于 ireader先关基础库实现的http-flv功能  

# 实现基础
* 1，依赖https://github.com/ireader/sdk.git ， 网络及系统相关基础库实现  
* 2，依赖https://github.com/ireader/media-server.git ， 基于sdk基础库实现的相关媒体库，包括(librtp/librtsp)  
* 3，依赖libringbuf，自实现的一个ringbuf库，用来存放音视频数据缓冲  
* 4，依赖media_demuxer.c，用于ffmpeg读取h264/h265等文件的自实现库（test时才是用）  

# 已实现
* httpflv server API 接口功能  
* 支持H264/H264+AAC  


# 待实现
* 音频完善  
* 不同视频分辨率支持（目前测试手机视频不支持width < heigh）  


# HTTP-FLV协议
* 优酷、斗鱼、熊猫tv、虎牙和哔哩哔哩pc网页使用了http-flv;
* 原理：  
	* 使用了HTTP content-length字段特点，服务器回复客户端请求的时候不加content-length字段，在回复了http内容之后，紧接着发送flv数据，客户端就一直接收数据了  
	* 使用HTTP chunk编码传输数据，chunk编码规则自行百度，很简单  
	* ```
HTTP/1.1 200 OK
Server: HTTP-FLV/2.1.1
Content-Type: video/x-flv
Transfer-Encoding: chunked
Connection: keep-alive
Expires: -1
Access-Control-Allow-Origin: *
Access-Control-Allow-Credentials: true
	```

# Tips
* HTTP-FLV实时延时1~2s   
* 调试最好办法是找一个可用的播放链接VLC进行抓包对比分析  
* 音频：每帧音频数据都需要加上ATDS头  
* 视频：最好在直播前封装SPS/PPS帧到FLV中  



# TEST
* 基于ubuntu 64bit环境，把sdk/media-server源码clone到ireader-rtsp下  
* make
* sudo ./ ff_httpflvsrv  
* VLC打开链接(http://ip:554/live/0)播放视频  




