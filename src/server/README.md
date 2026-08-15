# Server Orchestration

Server 编排层负责把三层串起来。

目标结构：

```text
CaptureThread
        ↓
FrameSlot
        ↓
EncodeThread
        ↓
SendQueue
        ↓
MediaTransportThread
```

计划文件：

```text
server_pipeline.h/.cpp
server_options.h/.cpp
```
