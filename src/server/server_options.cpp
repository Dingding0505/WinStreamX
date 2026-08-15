#include "server/server_options.h"

#include <charconv>
#include <string>
#include <string_view>

namespace winstreamx {
namespace {

bool HasValue(int index, int argc) {
    return index + 1 < argc;
}

Result<int> ParseInt(std::string_view text, const char* name) {
    int value = 0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return Status::Error(std::string("invalid integer for ") + name);
    }
    return value;
}

}  // namespace

Result<ServerOptions> ParseServerOptions(int argc, char** argv) {
    ServerOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.mode = ServerMode::Help;
            return options;
        }
        if (arg == "--version") {
            options.mode = ServerMode::Version;
            return options;
        }
        if (arg == "--pipeline") {
            options.mode = ServerMode::Pipeline;
            return options;
        }
        if (arg == "--mode") {
            if (!HasValue(i, argc)) {
                return Status::Error("--mode requires a value");
            }
            const std::string_view mode = argv[++i];
            if (mode == "capture") {
                options.mode = ServerMode::Capture;
            } else {
                return Status::Error("unsupported mode: " + std::string(mode));
            }
            continue;
        }
        if (arg == "--output") {
            if (!HasValue(i, argc)) {
                return Status::Error("--output requires a value");
            }
            options.output_path = argv[++i];
            continue;
        }
        if (arg == "--adapter") {
            if (!HasValue(i, argc)) {
                return Status::Error("--adapter requires a value");
            }
            auto parsed = ParseInt(argv[++i], "--adapter");
            if (!parsed.ok()) {
                return parsed.status();
            }
            options.adapter_index = parsed.value();
            continue;
        }
        return Status::Error("unknown argument: " + std::string(arg));
    }

    if (options.mode == ServerMode::Capture && options.output_path.empty()) {
        return Status::Error("--mode capture requires --output <path>");
    }
    if (options.adapter_index < 0) {
        return Status::Error("--adapter must be >= 0");
    }

    return options;
}

}  // namespace winstreamx
