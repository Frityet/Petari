#include "Game/Util/DirectDraw.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

static void require(bool value) {
    if (!value) throw std::runtime_error("original DirectDraw RGBA8 texture check failed");
}

int main() {
    // Sequential hardware tiles provide an independent layout oracle: each
    // 4x4 tile has an alpha/red plane followed by a green/blue plane.
    constexpr u32 width = 12;
    constexpr u32 height = 8;
    std::array<u8, width * height * 4> tiled{};
    std::array<u32, width * height> linear{};
    std::size_t tile = 0;
    for (u32 ty = 0; ty < height; ty += 4) {
        for (u32 tx = 0; tx < width; tx += 4, tile += 64) {
            for (u32 y = 0; y < 4; ++y) {
                for (u32 x = 0; x < 4; ++x) {
                    const u32 i = (ty + y) * width + tx + x;
                    const u8 red = static_cast<u8>(i * 37 + 0x12);
                    const u8 green = static_cast<u8>(i * 19 + 0x34);
                    const u8 blue = static_cast<u8>(i * 11 + 0x56);
                    const u8 alpha = static_cast<u8>(i * 7 + 0x78);
                    const auto sample = tile + 2 * (4 * y + x);
                    tiled[sample] = alpha;
                    tiled[sample + 1] = red;
                    tiled[sample + 32] = green;
                    tiled[sample + 33] = blue;
                    linear[i] = (u32(red) << 24) | (u32(green) << 16) | (u32(blue) << 8) | alpha;
                }
            }
        }
    }
    std::array<u8, tiled.size() + 64> encoded{};
    encoded.fill(0xa5);
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            const auto expected = linear[y * width + x];
            require(TDDraw::getTexel32(tiled.data(), width, x, y) == expected);
            TDDraw::setTexel32(encoded.data() + 32, width, x, y, expected);
        }
    }
    require(std::equal(tiled.begin(), tiled.end(), encoded.begin() + 32));
    require(std::all_of(encoded.begin(), encoded.begin() + 32, [](u8 b) { return b == 0xa5; }));
    require(std::all_of(encoded.end() - 32, encoded.end(), [](u8 b) { return b == 0xa5; }));
    std::cout << "DirectDraw: 96 independently encoded RGBA8 texels across six tiles; getter, channel order, writer, and guards pass\n";
}
