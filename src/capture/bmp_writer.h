#pragma once

#include <cstdint>
#include <string>

#include "core/result.h"

namespace winstreamx {

struct BgraImageView {
    const std::uint8_t* pixels = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride_bytes = 0;
};

Status WriteBgraBmp(const std::string& path, const BgraImageView& image);

}  // namespace winstreamx
