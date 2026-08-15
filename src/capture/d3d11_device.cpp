#include "capture/d3d11_device.h"

#include <array>

namespace winstreamx {

Result<D3D11DeviceContext> CreateD3D11DeviceForAdapter(int adapter_index) {
    if (adapter_index < 0) {
        return Status::Error("adapter index must be >= 0");
    }

    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return Status::Error("CreateDXGIFactory1 failed");
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    hr = factory->EnumAdapters1(static_cast<UINT>(adapter_index), &adapter);
    if (FAILED(hr)) {
        return Status::Error("failed to enumerate DXGI adapter");
    }

    constexpr std::array<D3D_FEATURE_LEVEL, 2> levels{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL selected_level = D3D_FEATURE_LEVEL_11_0;

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    hr = D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels.data(),
        static_cast<UINT>(levels.size()),
        D3D11_SDK_VERSION,
        &device,
        &selected_level,
        &context);
    if (FAILED(hr)) {
        return Status::Error("D3D11CreateDevice failed");
    }

    D3D11DeviceContext result;
    result.device = device;
    result.context = context;
    result.adapter = adapter;
    return result;
}

}  // namespace winstreamx
