#include "capture/bmp_writer.h"

#include <array>
#include <cstddef>
#include <fstream>

namespace winstreamx {
namespace {

void WriteU16(std::ofstream& output, std::uint16_t value) {
    const std::array<char, 2> bytes{
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
    };
    output.write(bytes.data(), bytes.size());
}

void WriteU32(std::ofstream& output, std::uint32_t value) {
    const std::array<char, 4> bytes{
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff),
    };
    output.write(bytes.data(), bytes.size());
}

}  // namespace

Status WriteBgraBmp(const std::string& path, const BgraImageView& image) {
    if (image.pixels == nullptr || image.width == 0 || image.height == 0) {
        return Status::Error("invalid BGRA image");
    }
    if (image.stride_bytes < image.width * 4) {
        return Status::Error("invalid BGRA stride");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Status::Error("failed to open BMP output path");
    }

    const std::uint32_t row_bytes = image.width * 4;
    const std::uint32_t pixel_bytes = row_bytes * image.height;
    const std::uint32_t file_header_bytes = 14;
    const std::uint32_t dib_header_bytes = 40;
    const std::uint32_t pixel_offset = file_header_bytes + dib_header_bytes;
    const std::uint32_t file_size = pixel_offset + pixel_bytes;

    output.put('B');
    output.put('M');
    WriteU32(output, file_size);
    WriteU16(output, 0);
    WriteU16(output, 0);
    WriteU32(output, pixel_offset);

    WriteU32(output, dib_header_bytes);
    WriteU32(output, image.width);
    WriteU32(output, image.height);
    WriteU16(output, 1);
    WriteU16(output, 32);
    WriteU32(output, 0);
    WriteU32(output, pixel_bytes);
    WriteU32(output, 0);
    WriteU32(output, 0);
    WriteU32(output, 0);
    WriteU32(output, 0);

    for (std::uint32_t row = 0; row < image.height; ++row) {
        const std::uint32_t src_y = image.height - 1 - row;
        const auto* src = image.pixels + static_cast<std::size_t>(src_y) * image.stride_bytes;
        output.write(reinterpret_cast<const char*>(src), row_bytes);
    }

    if (!output) {
        return Status::Error("failed to write BMP");
    }
    return Status::Ok();
}

}  // namespace winstreamx
