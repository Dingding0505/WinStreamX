#include "transport/transport_layer.h"

namespace winstreamx {

std::string_view TransportLayer::name() const {
    return "Media Transport Layer";
}

std::string_view TransportLayer::responsibility() const {
    return "AVPacket -> FFmpeg libavformat RTP/RTSP";
}

}  // namespace winstreamx
