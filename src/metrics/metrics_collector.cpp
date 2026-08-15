#include "metrics/metrics_collector.h"

namespace winstreamx {

std::string_view MetricsCollector::name() const {
    return "Metrics";
}

std::string_view MetricsCollector::responsibility() const {
    return "FPS, latency, queue depth, bitrate";
}

}  // namespace winstreamx
