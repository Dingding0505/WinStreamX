#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "capture/bmp_writer.h"

namespace {

int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

std::uint32_t ReadU32(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

void TestWriteBgraBmpHeader() {
    const char* path = "bmp_writer_test_output.bmp";
    const std::uint8_t pixels[] = {
        0, 0, 255, 255,
        0, 255, 0, 255,
    };
    const winstreamx::BgraImageView image{pixels, 2, 1, 8};

    const auto status = winstreamx::WriteBgraBmp(path, image);
    Expect(status.ok(), "bmp write should succeed");

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto file_size = input.tellg();
    input.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file_size));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    Expect(bytes.size() == 14 + 40 + 8, "bmp size should match 2x1 32-bit image");
    Expect(bytes[0] == 'B' && bytes[1] == 'M', "bmp signature");
    Expect(ReadU32(bytes, 18) == 2, "bmp width");
    Expect(ReadU32(bytes, 22) == 1, "bmp height");
    std::remove(path);
}

}  // namespace

int main() {
    TestWriteBgraBmpHeader();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
