#include <chrono>
#include <iostream>
#include <thread>

#include <windows.h>

#include "capture/bmp_writer.h"
#include "capture/d3d11_device.h"
#include "capture/desktop_duplicator.h"
#include "core/version.h"
#include "server/server_options.h"
#include "server/server_pipeline.h"

namespace {

class DesktopUpdateTrigger {
public:
    DesktopUpdateTrigger() {
        window_ = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            L"STATIC",
            L"WinStreamXCaptureTrigger",
            WS_POPUP,
            0,
            0,
            1,
            1,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);
        if (window_ != nullptr) {
            SetLayeredWindowAttributes(window_, 0, 1, LWA_ALPHA);
            ShowWindow(window_, SW_SHOWNOACTIVATE);
            UpdateWindow(window_);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    DesktopUpdateTrigger(const DesktopUpdateTrigger&) = delete;
    DesktopUpdateTrigger& operator=(const DesktopUpdateTrigger&) = delete;

    ~DesktopUpdateTrigger() {
        if (window_ != nullptr) {
            DestroyWindow(window_);
        }
    }

private:
    HWND window_ = nullptr;
};

void PrintHelp() {
    std::cout
        << "WinStreamX Server\n"
        << "Usage:\n"
        << "  winstreamx_server --version\n"
        << "  winstreamx_server --pipeline\n"
        << "  winstreamx_server --mode capture --output frame.bmp [--adapter 0]\n"
        << "  winstreamx_server --help\n";
}

void PrintPipeline() {
    winstreamx::ServerPipeline pipeline;
    std::cout << pipeline.Describe();
}

}  // namespace

int main(int argc, char** argv) {
    auto parsed = winstreamx::ParseServerOptions(argc, argv);
    if (!parsed.ok()) {
        std::cerr << "error: " << parsed.status().message() << "\n";
        PrintHelp();
        return 1;
    }

    const auto& options = parsed.value();
    switch (options.mode) {
    case winstreamx::ServerMode::Help:
        PrintHelp();
        return 0;
    case winstreamx::ServerMode::Version:
        std::cout << winstreamx::kProjectName << " " << winstreamx::kProjectVersion << "\n";
        return 0;
    case winstreamx::ServerMode::Pipeline:
        PrintPipeline();
        return 0;
    case winstreamx::ServerMode::Capture: {
        auto d3d = winstreamx::CreateD3D11DeviceForAdapter(options.adapter_index);
        if (!d3d.ok()) {
            std::cerr << "error: " << d3d.status().message() << "\n";
            return 1;
        }

        winstreamx::DesktopDuplicator duplicator(std::move(d3d.value()));
        const auto init_status = duplicator.Initialize();
        if (!init_status.ok()) {
            std::cerr << "error: " << init_status.message() << "\n";
            return 1;
        }

        std::cout << "capturing desktop frame...\n";
        DesktopUpdateTrigger desktop_update_trigger;
        auto frame = duplicator.CaptureFrame();
        if (!frame.ok()) {
            std::cerr << "error: " << frame.status().message() << "\n";
            return 1;
        }

        const winstreamx::BgraImageView image{
            frame.value().pixels.data(),
            frame.value().width,
            frame.value().height,
            frame.value().stride_bytes,
        };
        const auto write_status = winstreamx::WriteBgraBmp(options.output_path, image);
        if (!write_status.ok()) {
            std::cerr << "error: " << write_status.message() << "\n";
            return 1;
        }

        std::cout << "saved screenshot: " << options.output_path << "\n";
        return 0;
    }
    }

    return 1;
}
