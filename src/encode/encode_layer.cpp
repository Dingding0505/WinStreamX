#include "encode/encode_layer.h"

namespace winstreamx {

std::string_view EncodeLayer::name() const {
    return "Encode Layer";
}

std::string_view EncodeLayer::responsibility() const {
    return "BGRA/NV12/YUV420P -> H.264 AVPacket";
}

}  // namespace winstreamx
