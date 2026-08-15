#include "capture/capture_layer.h"

namespace winstreamx {

std::string_view CaptureLayer::name() const {
    return "Capture Layer";
}

std::string_view CaptureLayer::responsibility() const {
    return "Windows Desktop -> BGRA / D3D11 Texture";
}

}  // namespace winstreamx
