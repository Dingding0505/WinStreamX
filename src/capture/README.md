# Capture Layer

获取层负责从 Windows 获取桌面帧。

后续实现顺序：

```text
1. 创建 D3D11 device / context
2. 初始化 DXGI Desktop Duplication
3. 获取 ID3D11Texture2D 桌面帧
4. 第一版通过 staging texture 读回 BGRA
5. 后续保留 GPU Texture 给编码和 GPU 前处理使用
```

计划文件：

```text
d3d11_device.h/.cpp
desktop_duplicator.h/.cpp
captured_frame.h
```
