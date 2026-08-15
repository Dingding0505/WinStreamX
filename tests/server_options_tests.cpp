#include <cstdlib>
#include <iostream>

#include "server/server_options.h"

namespace {

int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

void TestVersionMode() {
    char app[] = "winstreamx_server";
    char version[] = "--version";
    char* argv[] = {app, version};

    const auto result = winstreamx::ParseServerOptions(2, argv);
    Expect(result.ok(), "version mode parse should succeed");
    Expect(result.value().mode == winstreamx::ServerMode::Version, "version mode");
}

void TestCaptureModeRequiresOutput() {
    char app[] = "winstreamx_server";
    char mode[] = "--mode";
    char capture[] = "capture";
    char* argv[] = {app, mode, capture};

    const auto result = winstreamx::ParseServerOptions(3, argv);
    Expect(!result.ok(), "capture mode without output should fail");
}

void TestCaptureModeWithOutput() {
    char app[] = "winstreamx_server";
    char mode[] = "--mode";
    char capture[] = "capture";
    char output[] = "--output";
    char path[] = "frame.bmp";
    char* argv[] = {app, mode, capture, output, path};

    const auto result = winstreamx::ParseServerOptions(5, argv);
    Expect(result.ok(), "capture mode with output should succeed");
    Expect(result.value().mode == winstreamx::ServerMode::Capture, "capture mode");
    Expect(result.value().output_path == "frame.bmp", "capture output path");
}

}  // namespace

int main() {
    TestVersionMode();
    TestCaptureModeRequiresOutput();
    TestCaptureModeWithOutput();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
