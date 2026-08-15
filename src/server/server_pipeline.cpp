#include "server/server_pipeline.h"

#include <sstream>

#include "capture/capture_layer.h"
#include "diagnostics/logger.h"
#include "encode/encode_layer.h"
#include "frame/frame_pipeline.h"
#include "metrics/metrics_collector.h"
#include "transport/transport_layer.h"

namespace winstreamx {

std::string ServerPipeline::Describe() const {
    const CaptureLayer capture;
    const FramePipelineLayer frame_pipeline;
    const EncodeLayer encode;
    const TransportLayer transport;
    const MetricsCollector metrics;
    const Logger diagnostics;

    std::ostringstream output;
    output << "WinStreamX Server-first Architecture\n"
           << "Layer 1. Server App & Runtime      : entry, options, lifecycle\n"
           << "Layer 2. " << capture.name() << "             : "
           << capture.responsibility() << "\n"
           << "Layer 3. " << frame_pipeline.name() << "      : "
           << frame_pipeline.responsibility() << "\n"
           << "Layer 4. " << encode.name() << "              : "
           << encode.responsibility() << "\n"
           << "Layer 5. " << transport.name() << "     : "
           << transport.responsibility() << "\n"
           << "Layer 6. Supporting Infrastructure : "
           << metrics.name() << ", " << diagnostics.name() << ", common utilities\n";
    return output.str();
}

}  // namespace winstreamx
