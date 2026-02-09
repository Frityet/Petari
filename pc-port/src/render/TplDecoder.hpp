#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace pcport {

enum class TplTextureFormat : std::uint32_t {
    I4 = 0,
    I8 = 1,
    IA4 = 2,
    IA8 = 3,
    RGB565 = 4,
    RGB5A3 = 5,
    RGBA32 = 6,
    C4 = 8,
    C8 = 9,
    C14X2 = 10,
    CMPR = 14,
};

struct ImageRGBA {
    int width = 0;
    int height = 0;
    TplTextureFormat format = TplTextureFormat::RGBA32;
    std::vector<std::uint8_t> pixels;

    bool Empty() const { return width <= 0 || height <= 0 || pixels.empty(); }
};

class TplDecoder {
public:
    static ImageRGBA DecodeFile(const std::filesystem::path& path);
};

}  // namespace pcport
