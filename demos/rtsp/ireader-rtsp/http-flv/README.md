
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
* 改善实时性：控制时间戳，让每一帧的时间戳间隔缩短，比如帧率25fps，可以次时间戳增量未38，这样能改善因为网络延时等因素引起的延时 --- 未验证  

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
* 音频：每帧音频数据都需要加上ATDS头，不然音频解码不出来  
* 视频：最好在直播前封装SPS/PPS帧到FLV中  
* 延时问题：1~2s，开始达到5s延时，原因：FLV解码时会严格按照时间戳来，同时VLC在播放HTTPF-LV视频时需要大致5s左右的视频数据才开始解码，所以导致一直保留5s+的延时  
	* 解决5s延时：
	* 在VLC请求建立HTTP-FLV连接时，音频数据从5s前数据开始发送，加快VLC出图时间  
	* 在VLC请求建立HTTP-FLV连接时，将最开始6s音视频的时间戳篡改，要求6s的数据1s内播放完  
* `aio_tcp_transport`接口中recv默认是短连接，recv完就会超时（默认接受超时4min，发送2min）关闭socket，所以长连接socket就得在recv回调处理函数一直处理（如不断发送flv数据），不要放在线程中处理  


# TEST
* 基于ubuntu 64bit环境，把sdk/media-server源码clone到ireader-rtsp下  
* make
* sudo ./ ff_httpflvsrv  
* VLC打开链接(http://ip:554/live/0)播放视频  




