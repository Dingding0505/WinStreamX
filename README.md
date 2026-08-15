# WinStreamX

WinStreamX 是一个 Windows 用户态图形采集、H.264 编码和 RTP/RTSP 传输项目。当前重新从最小 Server 端开始实现，先把 Server 端架构跑清楚。

最简单的业务主线是：

```text
Capture Layer
    获取 Windows 桌面帧
        ↓
Encode Layer
    BGRA / RGB -> H.264 AVPacket
        ↓
Transport Layer
    FFmpeg libavformat -> RTP / RTSP 输出
```

工程实现按 6 层组织：

```text
Layer 1: Server App & Runtime
Layer 2: Capture Layer
Layer 3: Frame Pipeline Layer
Layer 4: Encode Layer
Layer 5: Media Transport Layer
Layer 6: Supporting Infrastructure
```

## 当前阶段

当前只保留最小工程骨架：

```text
apps/winstreamx_server
src/core
src/capture
src/frame
src/encode
src/transport
src/server
src/metrics
src/diagnostics
docs
```

后续先做 Server，再做 Client。

## 生成 Visual Studio 解决方案

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -G "Visual Studio 18 2026" -A x64
```

打开：

```text
E:\exercitation\WinStreamX\build\WinStreamX.slnx
```

## 编译

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

## 运行

```powershell
.\build\Debug\winstreamx_server.exe --version
.\build\Debug\winstreamx_server.exe --pipeline
```

## 文档

```text
docs/winstreamx_architecture.md
docs/winstreamx_pipeline.md
docs/server_first_architecture.md
```
