#pragma once

#include <string>

#include "core/result.h"

namespace winstreamx {

enum class ServerMode {
    Help,
    Version,
    Pipeline,
    Capture,
};

struct ServerOptions {
    ServerMode mode = ServerMode::Help;
    std::string output_path;
    int adapter_index = 0;
};

Result<ServerOptions> ParseServerOptions(int argc, char** argv);

}  // namespace winstreamx
