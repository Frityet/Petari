#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace smgpc::render::layout {

struct TextureRef {
    std::uint64_t id {};
    const std::uint8_t *rgba8 {};
    std::uint16_t width {};
    std::uint16_t height {};
};

struct QuadCommand {
    float x0 {};
    float y0 {};
    float x1 {};
    float y1 {};

    float u0 {};
    float v0 {};
    float u1 {};
    float v1 {};

    std::uint32_t color_tl {};
    std::uint32_t color_tr {};
    std::uint32_t color_bl {};
    std::uint32_t color_br {};

    TextureRef texture {};
};

class LayoutDrawList {
public:
    void clear() {
        _quads.clear();
    }

    void reserve(std::size_t count) {
        _quads.reserve(count);
    }

    void push_quad(const QuadCommand &quad) {
        _quads.push_back(quad);
    }

    [[nodiscard]] const std::vector<QuadCommand> &quads() const {
        return _quads;
    }

private:
    std::vector<QuadCommand> _quads {};
};

[[nodiscard]] inline std::uint32_t pack_abgr(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    return (static_cast<std::uint32_t>(a) << 24U) |
           (static_cast<std::uint32_t>(b) << 16U) |
           (static_cast<std::uint32_t>(g) << 8U) |
           static_cast<std::uint32_t>(r);
}

}  // namespace smgpc::render::layout
