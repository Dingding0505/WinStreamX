#include "frame/frame_pipeline.h"

namespace winstreamx {

std::string_view FramePipelineLayer::name() const {
    return "Frame Pipeline Layer";
}

std::string_view FramePipelineLayer::responsibility() const {
    return "FrameSlot, SendQueue, frame history";
}

}  // namespace winstreamx
