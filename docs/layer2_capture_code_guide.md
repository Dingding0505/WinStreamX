﻿﻿# WinStreamX Layer 2：Capture Layer 代码学习文档

本文档用于帮助你学习当前已经跑通的第二层：桌面采集层。  
当前版本的目标不是录制视频，也不是网络传输，而是先把 Windows 桌面的一帧画面通过 DXGI / D3D11 采集出来，并保存成 BMP 图片。

当前已经验证通过的命令：

```powershell
.\build\Debug\winstreamx_server.exe --mode capture --output frame.bmp
```

运行成功时会看到：

```text
capturing desktop frame...
saved screenshot: frame.bmp
```

## 1. 当前 Layer 2 完成了什么

Layer 2 的职责是：

```text
Windows 桌面画面
        ↓
DXGI Desktop Duplication（API）
        ↓
ID3D11Texture2D（GPU里的图像）
        ↓
staging texture（CPU里的图像）
        ↓
CPU 内存中的 BGRA 像素（内存里的图像）
        ↓
BMP 文件
```

这一层已经覆盖了几个重要的 Windows 图形开发知识点：

| 技术点 | 当前项目里的体现 |
|---|---|
| DXGI Adapter | 通过 `IDXGIFactory1::EnumAdapters1` 枚举显卡适配器 |
| D3D11 Device | 通过 `D3D11CreateDevice` 创建 D3D11 设备 |
| D3D11 Context | 用于提交 `CopyResource`、`Map` 等 GPU 命令 |
| Desktop Duplication | 通过 `IDXGIOutputDuplication::AcquireNextFrame` 获取桌面帧 |
| D3D11 Texture | 桌面帧首先以 `ID3D11Texture2D` 的形式存在 |
| staging texture | 用于把 GPU 纹理读回 CPU |
| BGRA | 当前桌面帧在 CPU 内存中的像素格式 |
| BMP Writer | 把 BGRA 像素写成可查看的图片文件 |

## 2. 建议读代码顺序

建议你按这个顺序看，不要一上来钻进 DXGI 细节：

```text
1. apps/winstreamx_server/main.cpp
   先看程序如何进入 capture 模式

2. src/server/server_options.h / .cpp
   看命令行参数如何解析

3. src/capture/d3d11_device.h / .cpp
   看 D3D11 device/context/adapter 如何创建

4. src/capture/desktop_duplicator.h / .cpp
   看桌面帧如何被 DXGI 抓出来

5. src/capture/captured_frame.h
   看采集出来的帧在项目里怎么表示

6. src/capture/bmp_writer.h / .cpp
   看 BGRA 像素如何保存成 BMP

7. tests/bmp_writer_tests.cpp
   看最小单元测试怎么验证 BMP 写文件
```

你现在学习的重点是：**main 函数如何把命令行、D3D11 初始化、桌面采集、BMP 保存串起来。**

## 3. main.cpp：程序入口做了什么

文件：

```text
apps/winstreamx_server/main.cpp
```

核心逻辑：

```cpp
auto parsed = winstreamx::ParseServerOptions(argc, argv);
```

这行负责解析命令行参数。比如你输入：

```powershell
--mode capture --output frame.bmp
```

解析后会得到：

```text
mode        = Capture
output_path = frame.bmp
adapter     = 0
```

然后 `main` 根据 `options.mode` 进入不同分支：

```cpp
switch (options.mode) {
case winstreamx::ServerMode::Help:
case winstreamx::ServerMode::Version:
case winstreamx::ServerMode::Pipeline:
case winstreamx::ServerMode::Capture:
}
```

当前和 Layer 2 有关的是 `Capture` 分支。

## 4. Capture 分支的数据流

`main.cpp` 中 capture 模式的执行顺序是：

```text
1. CreateD3D11DeviceForAdapter()
   创建 D3D11 device/context/adapter

2. DesktopDuplicator duplicator(...)
   创建桌面复制对象

3. duplicator.Initialize()
   初始化 DXGI Desktop Duplication

4. DesktopUpdateTrigger
   主动制造一次很小的桌面变化，避免静态桌面没有新帧

5. duplicator.CaptureFrame()
   获取一帧桌面图像

6. WriteBgraBmp()
   保存为 BMP 文件
```

可以把它理解成：

```text
main.cpp 负责调度流程
D3D11Device 负责准备 GPU/DXGI 环境
DesktopDuplicator 负责拿到桌面帧
BmpWriter 负责把帧落盘
```

## 5. D3D11Device：为什么需要先创建设备

文件：

```text
src/capture/d3d11_device.h
src/capture/d3d11_device.cpp
```

核心结构：

```cpp
struct D3D11DeviceContext {
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
};
```

这三个对象分别是：

| 对象 | 作用 |
|---|---|
| `ID3D11Device` | 创建 D3D11 资源，比如 texture |
| `ID3D11DeviceContext` | 提交 GPU 命令，比如 copy、map |
| `IDXGIAdapter1` | 表示一块显卡或显示适配器 |

### 5.1 CreateDXGIFactory1

```cpp
CreateDXGIFactory1(IID_PPV_ARGS(&factory));
```

作用：创建 DXGI 工厂对象。

可以理解为：  
**你要先拿到 DXGI 的入口，才能枚举显卡、显示器等资源。**

### 5.2 EnumAdapters1

```cpp
factory->EnumAdapters1(static_cast<UINT>(adapter_index), &adapter);
```

作用：选择第几个显卡适配器。

当前默认：

```text
adapter_index = 0
```

也就是使用系统里的第一个适配器。

后续如果遇到多显卡、核显/独显、远程桌面等场景，就可能需要通过 `--adapter` 选择不同 adapter。

### 5.3 D3D11CreateDevice

```cpp
D3D11CreateDevice(...);
```

作用：基于指定 adapter 创建 D3D11 设备和上下文。

当前使用的关键参数：

```cpp
D3D_DRIVER_TYPE_UNKNOWN
D3D11_CREATE_DEVICE_BGRA_SUPPORT
D3D_FEATURE_LEVEL_11_1
D3D_FEATURE_LEVEL_11_0
```

重点理解：

| 参数 | 含义 |
|---|---|
| `D3D_DRIVER_TYPE_UNKNOWN` | 因为我们已经指定了 adapter，所以 driver type 用 UNKNOWN |
| `D3D11_CREATE_DEVICE_BGRA_SUPPORT` | 允许 BGRA 纹理格式，桌面采集和后续渲染常用 |
| `D3D_FEATURE_LEVEL_11_1 / 11_0` | 希望使用 D3D11.1，不行就退到 D3D11.0 |

这一步成功后，后面的 Desktop Duplication 才能工作。

## 6. DesktopDuplicator：真正抓桌面帧的地方

文件：

```text
src/capture/desktop_duplicator.h
src/capture/desktop_duplicator.cpp
```

核心类：

```cpp
class DesktopDuplicator {
public:
    explicit DesktopDuplicator(D3D11DeviceContext d3d);

    Status Initialize();
    Result<CapturedFrameBgra> CaptureFrame();
};
```

它有两个主要阶段：

```text
Initialize：准备 DXGI Desktop Duplication
CaptureFrame：真正获取一帧
```

## 7. Initialize：初始化 Desktop Duplication

关键代码：

```cpp
d3d_.adapter->EnumOutputs(0, &output);
```

作用：从当前 adapter 上枚举第一个显示输出。

这里的 `Output` 可以粗略理解为显示器输出。当前先写死为 `0`，也就是主输出。

之后：

```cpp
output.As(&output1);
```

作用：把 `IDXGIOutput` 转成 `IDXGIOutput1`。

为什么要转？  
因为 `DuplicateOutput` 是 `IDXGIOutput1` 提供的接口。

最后：

```cpp
output1->DuplicateOutput(d3d_.device.Get(), &duplication_);
```

作用：创建桌面复制对象。

成功后，我们就拿到了：

```cpp
IDXGIOutputDuplication
```

这个对象就是后续抓屏的核心。

## 8. CaptureFrame：一帧是怎么被拿出来的（核心）

核心流程：

```text
AcquireNextFrame
        ↓
IDXGIResource
        ↓
ID3D11Texture2D
        ↓
staging texture
        ↓
CopyResource
        ↓
Map
        ↓
复制到 CapturedFrameBgra.pixels
        ↓
Unmap / ReleaseFrame
```

### 8.1 AcquireNextFrame

```cpp
duplication_->AcquireNextFrame(1000, &frame_info, &resource);
```

作用：等待并获取下一帧桌面更新。

参数 `1000` 表示最多等待 1000 ms。

注意：Desktop Duplication 获取的是“桌面变化帧”。如果桌面完全静止，有时候可能等不到新帧。所以 `main.cpp` 里加了一个 `DesktopUpdateTrigger`，创建一个 1x1 的临时透明窗口，用来触发桌面更新。

### 8.2 IDXGIResource 转 ID3D11Texture2D

```cpp
resource.As(&texture);
```

`AcquireNextFrame` 返回的是通用的 `IDXGIResource`，但我们后续要用 D3D11 操作纹理，所以要转成：

```cpp
ID3D11Texture2D
```

这一步成功后，桌面帧就变成了一张 D3D11 纹理。

## 9. 同一帧图像的生命周期：从 DXGI 到 WinStreamX

在当前截图功能里，`IDXGIResource`、`ID3D11Texture2D`、`CapturedFrameBgra` 都可以理解为“同一帧桌面图像”，但它们属于不同层次、不同接口体系。

以一次截图为例：

```text
Windows 桌面当前画面
        ↓
DXGI Desktop Duplication
        ↓
IDXGIResource
        ↓ QueryInterface / ComPtr::As
ID3D11Texture2D
        ↓ CopyResource
staging texture
        ↓ Map
CPU 可读内存
        ↓ 按 RowPitch 逐行复制
CapturedFrameBgra
        ↓ WriteBgraBmp
frame.bmp
```

可以用一个简单比喻理解：

```text
IDXGIResource     ：快递系统里的包裹编号，说明“有这么一件资源”
ID3D11Texture2D   ：仓库里真正的一箱货，可以用 D3D11 的工具搬运、查询
CapturedFrameBgra ：拆箱后放到自己桌上的物品，可以按自己的项目逻辑处理
```

### 9.1 IDXGIResource：DXGI 世界里的通用资源

来源：

```cpp
duplication_->AcquireNextFrame(1000, &frame_info, &resource);
```

`IDXGIResource` 来自 Windows 的 DXGI Desktop Duplication API。

它表示：

```text
DXGI 返回给我们的这一帧桌面资源
```

特点：

```text
1. 它是 DXGI 层的通用资源接口
2. 它本身不直接暴露像素数组
3. 它通常仍然代表 GPU 侧资源
4. 它适合作为 Desktop Duplication API 的返回值
```

为什么不能直接拿它写 BMP？

因为 `IDXGIResource` 不是一块普通 CPU 内存。它只是 DXGI 给你的资源接口，不能直接像数组一样访问每个像素。

### 9.2 ID3D11Texture2D：D3D11 世界里的 2D 纹理

来源：

```cpp
Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
hr = resource.As(&texture);
```

这里的 `As` 本质上是 COM 的 `QueryInterface`。

意思是：

```text
我现在手里有 DXGI 的 resource。
我想问它背后的真实对象：
你能不能以 D3D11 2D Texture 的接口形式给我使用？
```

如果成功，`texture` 和 `resource` 指向的是同一个底层资源，只是接口不同。

`ID3D11Texture2D` 的特点：

```text
1. 它是 D3D11 层的 2D GPU 纹理
2. 可以调用 GetDesc 查询宽高、格式等信息
3. 可以作为 CopyResource 的源纹理
4. CPU 仍然不能直接读它的像素
```

为什么要从 `IDXGIResource` 转成 `ID3D11Texture2D`？

因为后续要使用 D3D11 API 操作这一帧：

```cpp
texture->GetDesc(&desc);
d3d_.context->CopyResource(staging.Get(), texture.Get());
```

这些操作需要 D3D11 纹理接口，而不是 DXGI 通用资源接口。

### 9.3 staging texture：GPU 到 CPU 的中转站

来源：

```cpp
D3D11_TEXTURE2D_DESC staging_desc = desc;
staging_desc.BindFlags = 0;
staging_desc.MiscFlags = 0;
staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
staging_desc.Usage = D3D11_USAGE_STAGING;

d3d_.device->CreateTexture2D(&staging_desc, nullptr, &staging);
```

staging texture 也是 `ID3D11Texture2D`，但它的用途不同。

原始桌面 texture 偏向 GPU 使用；staging texture 是专门用来让 CPU 读数据的。

```text
原始 texture：
    桌面采集得到的 GPU 纹理
    适合 GPU 使用
    CPU 不能直接读

staging texture：
    我们自己创建的中转纹理
    Usage = D3D11_USAGE_STAGING
    CPUAccessFlags = D3D11_CPU_ACCESS_READ
    可以被 Map 到 CPU 地址空间
```

为什么要多一次 `CopyResource`？

因为要把原始 GPU 纹理复制到 CPU 可读的 staging texture：

```cpp
d3d_.context->CopyResource(staging.Get(), texture.Get());
```

可以理解成：

```text
把仓库里的货，搬到一个允许你打开检查的临时工作台上。
```

### 9.4 CPU 可读内存：Map 之后才能看到像素

来源：

```cpp
D3D11_MAPPED_SUBRESOURCE mapped{};
hr = d3d_.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
```

`Map` 成功后，CPU 可以通过：

```cpp
mapped.pData
```

读取像素数据。

但这里还不是最终的项目内部帧，因为 `mapped.pData` 有一个重要问题：

```text
它每一行的实际跨度是 mapped.RowPitch
RowPitch 不一定等于 width * 4
```

所以不能简单地把整块内存当成连续紧凑图像处理。

### 9.5 CapturedFrameBgra：WinStreamX 项目内部的一帧

来源：

```cpp
CapturedFrameBgra frame;
frame.width = desc.Width;
frame.height = desc.Height;
frame.stride_bytes = desc.Width * 4;
frame.pixels.resize(static_cast<std::size_t>(frame.stride_bytes) * frame.height);
```

`CapturedFrameBgra` 是我们自己定义的结构体，不是 Windows 定义的。

它表示：

```text
WinStreamX 项目内部使用的一帧 CPU 图像
```

当前存储方式：

```text
pixels       ：连续的 BGRA 像素数组
width        ：图像宽度
height       ：图像高度
stride_bytes ：项目内部每一行字节数，当前等于 width * 4
```

为什么还要从 `mapped.pData` 复制到 `CapturedFrameBgra`？

因为我们希望后续模块拿到的是一个稳定、紧凑、由项目自己管理生命周期的数据结构。

如果后续直接依赖 `mapped.pData`，会有几个问题：

```text
1. Unmap 之后 mapped.pData 就不能再用了
2. RowPitch 可能包含 GPU 对齐填充
3. 后续 Pipeline / Encoder 不应该依赖 D3D11 Map 的临时内存
4. 项目内部需要自己的 frame_id、timestamp、format 等字段
```

所以当前版本选择复制成：

```cpp
CapturedFrameBgra
```

这是一种更清晰的边界：

```text
Windows / D3D11 资源
        ↓
WinStreamX 自己的帧对象
```

### 9.6 为什么要经历这么多次转换

这些转换不是为了复杂，而是因为每一层解决的问题不同：

| 阶段 | 属于谁 | 存在哪里 | 主要作用 | 为什么要转换 |
|---|---|---|---|---|
| `IDXGIResource` | DXGI | GPU 资源接口 | Desktop Duplication 返回一帧 | 只是通用资源，不能直接用 D3D11 操作 |
| `ID3D11Texture2D` | D3D11 | GPU 纹理 | 查询纹理信息，执行 GPU copy | CPU 不能直接读 |
| `staging texture` | D3D11 | GPU / CPU 中转纹理 | 允许 CPU readback | 原始纹理不可直接 Map |
| `mapped.pData` | D3D11 Map | CPU 可读临时地址 | 临时读取像素 | 生命周期受 Map / Unmap 控制，RowPitch 不固定 |
| `CapturedFrameBgra` | WinStreamX | CPU vector | 项目内部稳定帧对象 | 后续 Pipeline / Encode 需要自己的数据结构 |
| `frame.bmp` | 文件系统 | 磁盘 | 验证截图结果 | 方便人工检查采集是否正确 |

这条生命周期是 Layer 2 最核心的理解：

```text
DXGI 负责“给我桌面帧”
D3D11 负责“让我操作 GPU 纹理”
staging texture 负责“让我把 GPU 数据读回 CPU”
CapturedFrameBgra 负责“让项目内部统一处理一帧图像”
BMP 负责“让人能看到验证结果”
```

## 10. 为什么需要 staging texture

Desktop Duplication 得到的 `ID3D11Texture2D` 在 GPU 侧。  
CPU 不能直接像访问普通数组一样读它。

所以当前版本做了一个 staging texture：

```cpp
staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
staging_desc.Usage = D3D11_USAGE_STAGING;
staging_desc.BindFlags = 0;
staging_desc.MiscFlags = 0;
```

这表示：

```text
这个 texture 不用于渲染
这个 texture 允许 CPU 读取
这个 texture 用作 GPU -> CPU 数据中转
```

然后：

```cpp
d3d_.context->CopyResource(staging.Get(), texture.Get());
```

把 GPU 里的桌面纹理复制到 staging texture。

再通过：

```cpp
d3d_.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
```

把 staging texture 映射到 CPU 可读内存。

这就是当前项目里最关键的一条 GPU 到 CPU 链路。

## 11. RowPitch 为什么不能忽略

`Map` 之后得到：

```cpp
D3D11_MAPPED_SUBRESOURCE mapped;
```

里面有两个重要字段：

```cpp
mapped.pData
mapped.RowPitch
```

`pData` 是 CPU 可读的像素起始地址。  
`RowPitch` 是 GPU 纹理每一行实际占用的字节数。

注意：

```text
RowPitch 不一定等于 width * 4
```

因为 GPU 资源可能有内存对齐。

所以代码不能一次性整块 memcpy，而是按行复制：

```cpp
for (std::uint32_t y = 0; y < frame.height; ++y) {
    const auto* src_row = src + y * mapped.RowPitch;
    auto* dst_row = frame.pixels.data() + y * frame.stride_bytes;
    std::copy(src_row, src_row + frame.stride_bytes, dst_row);
}
```

这里项目自己的 `frame.stride_bytes` 固定成：

```cpp
width * 4
```

因为当前输出的是紧凑 BGRA 数据，每个像素 4 字节。

## 12. CapturedFrameBgra：项目内部怎么表示一帧

文件：

```text
src/capture/captured_frame.h
```

结构：

```cpp
struct CapturedFrameBgra {
    std::vector<std::uint8_t> pixels;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride_bytes = 0;
};
```

当前它保存的是：

| 字段 | 含义 |
|---|---|
| `pixels` | CPU 内存中的 BGRA 像素 |
| `width` | 图像宽度 |
| `height` | 图像高度 |
| `stride_bytes` | 每行字节数 |

后续进入 Layer 3 时，这个结构会继续扩展，加入：

```text
frame_id
capture_timestamp
format
dirty_rects
```

这些字段会用于帧同步、延迟统计、丢帧分析。

## 13. BmpWriter：为什么当前先保存 BMP

文件：

```text
src/capture/bmp_writer.h
src/capture/bmp_writer.cpp
```

BMP 的作用不是最终功能，而是第一阶段的验证工具。

它让我们确认：

```text
桌面真的被采集到了
宽高是对的
BGRA 像素顺序是对的
GPU -> CPU copy 是对的
```

`WriteBgraBmp` 接收：

```cpp
struct BgraImageView {
    const std::uint8_t* pixels;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t stride_bytes;
};
```

这是一个“视图对象”，它不拥有像素内存，只是告诉 BMP writer：

```text
像素在哪里
宽高是多少
每行多少字节
```

BMP 文件是倒着存行的，所以写文件时使用：

```cpp
const std::uint32_t src_y = image.height - 1 - row;
```

这表示从图像最后一行开始写，否则图片会上下颠倒。

## 14. DesktopUpdateTrigger 是什么

文件：

```text
apps/winstreamx_server/main.cpp
```

类：

```cpp
class DesktopUpdateTrigger
```

它创建了一个非常小的透明窗口：

```text
1x1 像素
透明
不抢焦点
工具窗口
置顶
```

目的不是显示 UI，而是触发一次桌面更新。

原因是：

```text
DXGI Desktop Duplication 通常等待桌面发生变化
如果桌面完全静止，AcquireNextFrame 可能超时
```

这个小工具让截图命令在静态桌面下也更稳定。

## 15. 错误处理方式

当前项目没有直接抛异常，而是用：

```cpp
Status
Result<T>
```

文件：

```text
src/core/result.h
```

例如：

```cpp
Result<D3D11DeviceContext> CreateD3D11DeviceForAdapter(int adapter_index)
```

成功时返回：

```text
Result 里包含 D3D11DeviceContext
```

失败时返回：

```text
Status::Error("错误信息")
```

`main.cpp` 里统一判断：

```cpp
if (!d3d.ok()) {
    std::cerr << "error: " << d3d.status().message() << "\n";
    return 1;
}
```

这种写法适合底层系统项目，因为错误路径比较明确，调试时也容易知道失败在哪一步。

## 16. 当前阶段你需要重点掌握的方法

第一优先级：

| 方法 | 为什么重要 |
|---|---|
| `CreateD3D11DeviceForAdapter` | D3D11 / DXGI 的入口 |
| `DesktopDuplicator::Initialize` | Desktop Duplication 初始化 |
| `DesktopDuplicator::CaptureFrame` | 真正的桌面帧采集链路 |
| `WriteBgraBmp` | 验证 BGRA 图像是否正确 |
| `ParseServerOptions` | 程序如何进入不同运行模式 |

第二优先级：

| 方法 / 概念 | 为什么重要 |
|---|---|
| `DesktopUpdateTrigger` | 解决静态桌面下拿不到帧的问题 |
| `ComPtr` | Windows COM 对象自动释放 |
| `HRESULT` | Windows API 的错误返回方式 |
| `RowPitch` | GPU 图像内存对齐问题 |
| `staging texture` | GPU 到 CPU readback 的关键中间层 |

## 17. 当前阶段验收标准

你能够做到以下几件事，就说明 Layer 2 真的学会了：

```text
1. 能解释为什么要先创建 D3D11 device/context
2. 能解释 adapter、output、duplication 分别是什么
3. 能解释 AcquireNextFrame 返回的是什么
4. 能解释为什么要把 IDXGIResource 转成 ID3D11Texture2D
5. 能解释为什么不能直接读 GPU texture
6. 能解释 staging texture 的用途
7. 能解释 RowPitch 为什么不一定等于 width * 4
8. 能解释 BGRA 数据如何保存成 BMP
9. 能独立运行截图命令并找到输出图片
```

## 18. 面试时可以怎么讲 Layer 2

可以这样讲：

```text
在 WinStreamX 的采集层中，我使用 DXGI Desktop Duplication 获取 Windows 桌面帧。
程序首先枚举 DXGI adapter，并基于指定 adapter 创建 D3D11 device/context。
随后通过 IDXGIOutput1::DuplicateOutput 创建桌面复制对象，使用 AcquireNextFrame 获取桌面更新帧。
由于采集到的帧最初是 GPU 侧的 ID3D11Texture2D，CPU 不能直接访问，所以我创建了 D3D11_USAGE_STAGING 且带 D3D11_CPU_ACCESS_READ 的 staging texture，通过 CopyResource 完成 GPU 到 CPU 的拷贝，再用 Map 读取 BGRA 像素。
当前最小版本会把 BGRA 数据保存成 BMP，用来验证采集链路、像素格式和 RowPitch 处理是否正确。
```

如果面试官继续追问，可以重点回答：

```text
1. Desktop Duplication 和 GDI 截图有什么区别？
2. 为什么需要 staging texture？
3. RowPitch 为什么不能直接假设为 width * 4？
4. AcquireNextFrame 超时怎么办？
5. 当前 GPU -> CPU copy 对性能有什么影响？
6. 后续如何减少 readback，比如保留 GPU texture 或用 GPU 做 BGRA -> NV12？
```

## 19. 下一步 Layer 3 会接在哪里

Layer 3 不会替换 Layer 2，而是接在 `CapturedFrameBgra` 后面。

当前是：

```text
CaptureFrame()
        ↓
WriteBgraBmp()
```

下一阶段会变成：

```text
CaptureFrame()
        ↓
FrameSlot / RingBuffer
        ↓
MockEncode / 后续 H.264 Encode
```

也就是说，Layer 2 负责“拿到帧”，Layer 3 负责“让帧在多线程实时管线里流动”。
