#include "capture/desktop_duplicator.h"

#include <algorithm>
#include <utility>

namespace winstreamx {

DesktopDuplicator::DesktopDuplicator(D3D11DeviceContext d3d) : d3d_(std::move(d3d)) {}

Status DesktopDuplicator::Initialize() {
    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    HRESULT hr = d3d_.adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        return Status::Error("failed to enumerate primary DXGI output");
    }

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) {
        return Status::Error("IDXGIOutput1 query failed");
    }

    hr = output1->DuplicateOutput(d3d_.device.Get(), &duplication_);
    if (FAILED(hr)) {
        return Status::Error("DuplicateOutput failed; run in an interactive desktop session");
    }

    return Status::Ok();
}

Result<CapturedFrameBgra> DesktopDuplicator::CaptureFrame() {
    if (!duplication_) {
        return Status::Error("DesktopDuplicator is not initialized");
    }

    DXGI_OUTDUPL_FRAME_INFO frame_info{};
    Microsoft::WRL::ComPtr<IDXGIResource> resource;

    HRESULT hr = S_OK;
    for (int attempt = 0; attempt < 5; ++attempt) {
        resource.Reset();
        hr = duplication_->AcquireNextFrame(1000, &frame_info, &resource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            continue;
        }
        if (FAILED(hr)) {
            return Status::Error("AcquireNextFrame failed");
        }
        if (frame_info.AccumulatedFrames > 0) {
            break;
        }
        duplication_->ReleaseFrame();
    }

    if (!resource || frame_info.AccumulatedFrames == 0) {
        return Status::Error("timed out waiting for a desktop content frame");
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    hr = resource.As(&texture);
    if (FAILED(hr)) {
        duplication_->ReleaseFrame();
        return Status::Error("failed to query captured texture");
    }

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC staging_desc = desc;
    staging_desc.BindFlags = 0;
    staging_desc.MiscFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.Usage = D3D11_USAGE_STAGING;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    hr = d3d_.device->CreateTexture2D(&staging_desc, nullptr, &staging);
    if (FAILED(hr)) {
        duplication_->ReleaseFrame();
        return Status::Error("failed to create staging texture");
    }

    d3d_.context->CopyResource(staging.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = d3d_.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        duplication_->ReleaseFrame();
        return Status::Error("failed to map staging texture");
    }

    CapturedFrameBgra frame;
    frame.width = desc.Width;
    frame.height = desc.Height;
    frame.stride_bytes = desc.Width * 4;
    frame.pixels.resize(static_cast<std::size_t>(frame.stride_bytes) * frame.height);

    const auto* src = static_cast<const std::uint8_t*>(mapped.pData);
    for (std::uint32_t y = 0; y < frame.height; ++y) {
        const auto* src_row = src + static_cast<std::size_t>(y) * mapped.RowPitch;
        auto* dst_row = frame.pixels.data() + static_cast<std::size_t>(y) * frame.stride_bytes;
        std::copy(src_row, src_row + frame.stride_bytes, dst_row);
    }

    d3d_.context->Unmap(staging.Get(), 0);
    duplication_->ReleaseFrame();
    return frame;
}

}  // namespace winstreamx
