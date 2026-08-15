#pragma once

#include <cstdint>
#include <vector>

namespace winstreamx {

struct CapturedFrameBgra {
    std::vector<std::uint8_t> pixels;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride_bytes = 0;
};

}  // namespace winstreamx
