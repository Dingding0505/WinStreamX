# Transport Layer

发送层负责把 H.264 AVPacket 交给工业媒体传输栈。

正式路线：

```text
AVPacket / EncodedPacket
        ↓
FFmpeg libavformat muxer
        ↓
RTP / RTSP
```

调试路线：

```text
TCP framed H.264
```

TCP 调试模式只用于验证端到端正确性，不作为最终主线。

计划文件：

```text
media_transport.h
rtsp_media_sender.h/.cpp
tcp_debug_sender.h/.cpp
```
