#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "core/result.h"

namespace winstreamx {

struct D3D11DeviceContext {
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
};

Result<D3D11DeviceContext> CreateD3D11DeviceForAdapter(int adapter_index);

}  // namespace winstreamx
