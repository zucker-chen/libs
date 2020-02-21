
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
* 支持H264/H265视频传输  
* 支持多通道视频同时链接  
* 使用线程池，解决多用户连接点流出现卡顿问题  
* 支持rtsp server option/describe 方法认证，同时支持空用户名、空密码  
* 通过valgrind内存泄露测试   


# 待实现
* 音频传输  
* rtsp client API实现  

# Tips
* RTSP所谓的TCP/UDP连接方式，是指RTP音视频数据传输的方式，RTSP信令走HTTP方式及TCP通信，目前没有找到UDP方式client测试   
* SDP中`a=control:trackID=1` 表示上面的sdp描述属于哪个流，当有音视频时trackID用来区分音视频，DESCRIBE回复是用到，SETUP音频是会分别建立（所有会发送2次）  
* RTSP音视频传输时，UDP时音视频的数据是用单独的socket链接传输，所以socket会占用2组(rtp/rtcp)4个端口  


# TEST
* 基于ubuntu 64bit环境，把sdk/media-server源码clone到ireader-rtsp下  
* make
* sudo ./ ff_rtspsrv  
* VLC打开链接(rtsp://ip:554/live/0)播放视频  



# 附录
* RTSP 协议交互抓包情况  
```
OPTIONS rtsp://192.168.40.142:554/live/0 RTSP/1.0
CSeq: 2
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)

RTSP/1.0 200 OK
CSeq: 2
Date: Fri, 21 Feb 2020 01:08:22 GMT
Public: DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,ANNOUNCE,RECORD,GET_PARAMETER,SET_PARAMETER
Content-Length: 0

DESCRIBE rtsp://192.168.40.142:554/live/0 RTSP/1.0
CSeq: 3
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)
Accept: application/sdp

RTSP/1.0 200 OK
CSeq: 3
Date: Fri, 21 Feb 2020 01:08:22 GMT
Content-Type: application/sdp
Content-Length: 320

v=0
o=- 16283235071508697961 16283235071508697961 IN IP4 0.0.0.0
s=rtsp://192.168.40.142:554/live/0
c=IN IP4 0.0.0.0
t=0 0
a=range:npt=now-
a=recvonly
a=control:*
m=video 0 RTP/AVP 97
a=rtpmap:97 H264/90000
a=fmtp:97 profile-level-id=245;packetization-mode=1;
a=control:trackID=0
m=audio 0 RTP/AVP 0
a=control:trackID=1
SETUP rtsp://192.168.40.142:554/live/0/trackID=0 RTSP/1.0
CSeq: 4
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)
Transport: RTP/AVP;unicast;client_port=62244-62245

RTSP/1.0 200 OK
CSeq: 4
Date: Fri, 21 Feb 2020 01:08:22 GMT
Session: 0x7f6bac003900
Transport: RTP/AVP;unicast;client_port=62244-62245;server_port=54034-54035
Content-Length: 0

SETUP rtsp://192.168.40.142:554/live/0/trackID=1 RTSP/1.0
CSeq: 5
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)
Transport: RTP/AVP;unicast;client_port=62246-62247
Session: 0x7f6bac003900

RTSP/1.0 200 OK
CSeq: 5
Date: Fri, 21 Feb 2020 01:08:22 GMT
Session: 0x7f6bac003900
Transport: RTP/AVP;unicast;client_port=62246-62247;server_port=37086-37087
Content-Length: 0

PLAY rtsp://192.168.40.142:554/live/0 RTSP/1.0
CSeq: 6
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)
Session: 0x7f6bac003900
Range: npt=0.000-

RTSP/1.0 200 OK
CSeq: 6
Date: Fri, 21 Feb 2020 01:08:22 GMT
Session: 0x7f6bac003900
Range: npt=0.000-
RTP-Info: url=rtsp://192.168.40.142:554/live/0/trackID=0;seq=61820;rtptime=0,url=rtsp://192.168.40.142:554/live/0/trackID=1;seq=19847;rtptime=0
Content-Length: 0

GET_PARAMETER rtsp://192.168.40.142:554/live/0 RTSP/1.0
CSeq: 7
User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)
Session: 0x7f6bac003900

RTSP/1.0 200 OK
CSeq: 7
Date: Fri, 21 Feb 2020 01:09:20 GMT
Session: 0x7f6bac003900
Content-Length: 0

```


