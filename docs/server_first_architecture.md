# WinStreamX Server-first 架构分层

本文档参考成熟实时显示链路项目的文档组织方式，说明 WinStreamX Server 端的分层、模块职责、主数据流和阶段验收方式。

WinStreamX 不实现驱动，也不模拟硬件显示设备。它是一个 Windows 用户态项目，目标是先把 Server 端做清楚：

```text
Windows 桌面帧
        ↓
DXGI / D3D11 获取
        ↓
H.264 编码
        ↓
RTP / RTSP 输出
```

## 1. 总体分层

```text
┌──────────────────────────────────────────────────────────────┐
│ Layer 1: Server App & Runtime                                │
│          程序入口、命令行、生命周期管理                       │
├──────────────────────────────────────────────────────────────┤
│ Layer 2: Capture Layer                                       │
│          Windows 桌面帧获取，DXGI / D3D11                     │
├──────────────────────────────────────────────────────────────┤
│ Layer 3: Frame Pipeline Layer                                │
│          帧槽位、线程调度、时间戳、丢帧策略                    │
├──────────────────────────────────────────────────────────────┤
│ Layer 4: Encode Layer                                        │
│          像素格式转换，FFmpeg libavcodec H.264 编码           │
├──────────────────────────────────────────────────────────────┤
│ Layer 5: Media Transport Layer                               │
│          SendQueue，FFmpeg libavformat RTP / RTSP 输出        │
├──────────────────────────────────────────────────────────────┤
│ Layer 6: Supporting Infrastructure                           │
│          Result、日志、配置、Metrics、RingBuffer、诊断工具     │
└──────────────────────────────────────────────────────────────┘
```

可以把最简单的理解保留下来：

```text
获取层 -> 编码层 -> 发送层
```

但工程实现时会更细：

```text
App -> Capture -> Frame Pipeline -> Encode -> Media Transport -> Metrics/Diagnostics
```

## 2. Layer 1: Server App & Runtime

目录：

```text
apps/winstreamx_server/
src/server/
```

职责：

| 模块 | 职责 |
|------|------|
| `main.cpp` | 程序入口，解析 `--version`、`--pipeline`、后续 `--mode capture/record/stream` |
| `server_options` | 后续保存命令行参数，例如 fps、duration、codec、transport url |
| `server_pipeline` | 后续负责创建并启动 Capture / Encode / Transport 各线程 |

当前状态：

```text
winstreamx_server.exe --version
winstreamx_server.exe --pipeline
```

## 3. Layer 2: Capture Layer

目录：

```text
src/capture/
```

职责：

| 模块 | 职责 |
|------|------|
| `D3D11Device` | 创建 D3D11 device / context，枚举 DXGI adapter |
| `DesktopDuplicator` | 使用 DXGI Desktop Duplication 获取桌面帧 |
| `CapturedFrame` | 描述采集帧，包含 width、height、format、timestamp、CPU/GPU 数据 |
| `TextureReadback` | 第一版使用 staging texture 做 GPU -> CPU 读回 |

核心数据流：

```text
Windows Desktop
        ↓
DXGI Desktop Duplication
        ↓
ID3D11Texture2D
        ↓
staging texture
        ↓
CapturedFrame(BGRA)
```

第一阶段验收：

```text
能采集一帧桌面并保存 BMP。
```

后续优化：

```text
保留 GPU Texture
减少 GPU -> CPU copy
支持 dirty rect / move rect 信息
```

## 4. Layer 3: Frame Pipeline Layer

目录：

```text
src/frame/
src/server/
```

职责：

| 模块 | 职责 |
|------|------|
| `FrameSlot` | 连接 CaptureThread 和 EncodeThread，只保留最新帧 |
| `SendQueue` | 连接 EncodeThread 和 MediaTransportThread，有界队列 |
| `FrameHistoryRing` | 记录最近 N 帧的时间戳、状态和丢帧信息 |
| `ServerPipeline` | 管理线程启动、停止和 join |

关键设计：

```text
Capture -> Encode:
    不使用普通队列。
    使用 FrameSlot / condition_variable。
    编码线程慢时，新帧可以覆盖旧帧。

Encode -> Transport:
    使用有界 SendQueue。
    传输慢时，允许短暂缓冲。
    队列满时丢弃旧 packet，保留新 packet。
```

主线程模型：

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

第一阶段验收：

```text
MockCapture -> FrameSlot -> MockEncode -> SendQueue -> MockTransport
能输出 captured / encoded / sent / dropped 指标。
```

## 5. Layer 4: Encode Layer

目录：

```text
src/encode/
```

职责：

| 模块 | 职责 |
|------|------|
| `PixelConverter` | BGRA -> YUV420P / NV12 |
| `H264Encoder` | 封装 FFmpeg libavcodec H.264 编码 |
| `EncodedPacket` | 描述编码输出，包含 AVPacket、pts、dts、keyframe、timestamp |
| `EncodeStats` | 统计 encode_ms、bitrate、keyframe interval |

核心数据流：

```text
CapturedFrame(BGRA)
        ↓
PixelConverter
        ↓
YUV420P / NV12
        ↓
FFmpeg libavcodec
        ↓
H.264 AVPacket / EncodedPacket
```

第一阶段验收：

```text
把采集到的桌面帧编码为 .h264 或 .mp4。
文件可以用 ffplay / VLC 播放。
```

后续优化：

```text
zerolatency
GOP / bitrate / preset
BGRA -> NV12 GPU Compute Shader
```

## 6. Layer 5: Media Transport Layer

目录：

```text
src/transport/
```

职责：

| 模块 | 职责 |
|------|------|
| `MediaTransport` | 统一传输接口 |
| `RtspMediaSender` | 使用 FFmpeg libavformat 输出 RTP / RTSP |
| `TcpDebugSender` | 本地调试用 TCP framed H.264 |
| `TransportStats` | 统计 bitrate、send_ms、packet_count、jitter 等 |

正式路线：

```text
EncodedPacket / AVPacket
        ↓
FFmpeg libavformat muxer
        ↓
RTP / RTSP
```

调试路线：

```text
EncodedPacket
        ↓
TCP framed H.264
```

注意：

```text
TCP debug 只用于验证端到端正确性。
最终工业主线是 FFmpeg libavformat + RTP / RTSP。
```

第一阶段验收：

```text
使用 ffplay / VLC / ffmpeg 拉流验证 RTP / RTSP 输出。
```

## 7. Layer 6: Supporting Infrastructure

目录：

```text
src/core/
src/metrics/
src/diagnostics/
tests/
docs/
```

职责：

| 模块 | 职责 |
|------|------|
| `core` | Result、Status、version、后续配置解析 |
| `metrics` | FPS、耗时、码率、队列深度、丢帧率 |
| `diagnostics` | 日志、错误输出、dump、调试工具 |
| `tests` | 单元测试和 smoke test |
| `docs` | 架构、Pipeline、性能数据、面试材料 |

这层贯穿所有模块，不属于某一个单独业务流程。

## 8. Server 主数据流

```text
Windows Desktop
        ↓
[Layer 2] Capture Layer
        DXGI Desktop Duplication
        D3D11 Texture / BGRA Frame
        ↓
[Layer 3] Frame Pipeline
        FrameSlot
        frame_id / capture_ts
        ↓
[Layer 4] Encode Layer
        BGRA -> YUV420P / NV12
        FFmpeg libavcodec H.264
        ↓
[Layer 3] SendQueue / FrameHistoryRing
        encoded_ts / queue_depth / drop_count
        ↓
[Layer 5] Media Transport
        FFmpeg libavformat RTP / RTSP
        ↓
Network / Player / Future Client
```

## 9. Server 生命周期

```text
1. main()
   解析命令行
   创建 ServerOptions

2. ServerPipeline::Initialize()
   初始化 Capture Layer
   初始化 Encode Layer
   初始化 Transport Layer

3. ServerPipeline::Start()
   启动 CaptureThread
   启动 EncodeThread
   启动 MediaTransportThread

4. Running
   CaptureThread 采集帧
   EncodeThread 编码帧
   MediaTransportThread 输出 RTP / RTSP
   Metrics 周期打印

5. ServerPipeline::Stop()
   设置 stop flag
   唤醒阻塞线程
   join 所有线程
   flush 统计数据
```

## 10. 为什么这样设计

### 10.1 贴近实时显示链路

实时显示系统不能无限排队。

所以 WinStreamX 的设计是：

```text
原始帧阶段：
    只保留最新帧。

编码包阶段：
    有界队列缓冲。

拥塞时：
    丢旧帧 / 旧包，保留最新状态。
```

### 10.2 贴近工业音视频栈

协议层不重复造轮子：

```text
编码：
    FFmpeg libavcodec

封装传输：
    FFmpeg libavformat RTP / RTSP
```

### 10.3 贴近图形/GPU 岗位

项目中保留明确 GPU 图形链路：

```text
D3D11 Device / Context
DXGI Desktop Duplication
ID3D11Texture2D
staging texture
后续 Compute Shader BGRA -> NV12
```

## 11. 当前实现顺序

### Phase 0：最小 Server 工程

已完成：

```text
winstreamx_server.exe --version
winstreamx_server.exe --pipeline
```

### Phase 1：Capture Layer

目标：

```text
DXGI 截图保存 BMP。
```

验收：

```powershell
winstreamx_server --mode capture --output frame.bmp
```

### Phase 2：Frame Pipeline

目标：

```text
MockCapture -> FrameSlot -> MockEncode -> SendQueue -> MockTransport
```

验收：

```text
能看到 captured / encoded / sent / dropped。
```

### Phase 3：Encode Layer

目标：

```text
BGRA -> H.264
```

验收：

```text
生成 demo.h264 / demo.mp4。
```

### Phase 4：Transport Layer

目标：

```text
H.264 AVPacket -> RTP / RTSP
```

验收：

```text
ffplay / VLC 可以拉流播放。
```

### Phase 5：Client

目标：

```text
Receive -> Decode -> Render
```

验收：

```text
接收端窗口显示 Server 端桌面画面。
```
