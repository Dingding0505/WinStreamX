# Encode Layer

编码层负责将桌面帧转换为 H.264 码流。

后续实现顺序：

```text
1. MockEncode 验证线程边界
2. BGRA -> YUV420P
3. FFmpeg libavcodec H.264 编码
4. 输出 AVPacket / EncodedPacket
5. 统计 encode_ms、bitrate、keyframe
```

计划文件：

```text
encoded_packet.h
ffmpeg_h264_encoder.h/.cpp
pixel_converter.h/.cpp
```
