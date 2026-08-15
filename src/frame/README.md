# Frame Pipeline Layer

帧调度层负责连接采集、编码和媒体传输。

后续放置：

```text
frame_slot.h/.cpp
send_queue.h
frame_history_ring.h
frame_metadata.h
```

设计原则：

```text
Capture -> Encode:
    使用 FrameSlot，只保留最新帧。

Encode -> Transport:
    使用有界 SendQueue。

Metrics:
    记录 frame_id、capture_ts、encode_ts、send_ts、drop_count。
```
