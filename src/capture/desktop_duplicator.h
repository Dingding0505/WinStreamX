#pragma once

#include <cstdint>

#include <dxgi1_2.h>
#include <wrl/client.h>

#include "capture/captured_frame.h"
#include "capture/d3d11_device.h"
#include "core/result.h"

namespace winstreamx {

class DesktopDuplicator {
public:
    explicit DesktopDuplicator(D3D11DeviceContext d3d);

    Status Initialize();
    Result<CapturedFrameBgra> CaptureFrame();

private:
    D3D11DeviceContext d3d_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
};

}  // namespace winstreamx
