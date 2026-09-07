#include "JSystem/J2DGraph/J2DOrthoGraph.hpp"
#include "resource/GameResourceRuntime.hpp"
#include <aurora/aurora.h>

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    void near(float value, float expected) {
        if (!std::isfinite(value) || std::abs(value - expected) >= 0.000001f)
            throw std::runtime_error("J2D orthographic coefficient " + std::to_string(value) + " differs from " + std::to_string(expected));
    }
}

int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime resources;
        JUtility::TColor color(0x12345678);
        require(color.r == 0x12 && color.g == 0x34 && color.b == 0x56 && color.a == 0x78 &&
                    color.toUInt32() == 0x12345678, "JUT RGBA word/channel order");

        J2DOrthoGraph graph(3, 7, 320, 240, -100, 200);
        require(graph.getGrafType() == J2DGraf_Ortho, "actual orthographic virtual type");
        graph.setOrtho(TBox2f(-10, -20, 70, 40), -30, 90);
        graph.place(11, 13, 301, 203);
        graph.setPort();
        std::array<float, 6> viewport;
        GXGetViewportv(viewport.data());
        require(viewport == std::array<float, 6>{11, 13, 301, 203, 0, 1}, "J2D viewport follows placed framebuffer bounds");
        u32 left, top, width, height;
        GXGetScissor(&left, &top, &width, &height);
        require(left == 11 && top == 13 && width == 301 && height == 203, "J2D scissor follows original placement");
        near(graph.mMtx44[0][0], 2.0f / 80);
        near(graph.mMtx44[0][3], -60.0f / 80);
        near(graph.mMtx44[1][1], -2.0f / 60);
        near(graph.mMtx44[1][3], 20.0f / 60);
        near(graph.mMtx44[2][2], -1.0f / 120);
        near(graph.mMtx44[2][3], -30.0f / 120);

        std::cout << "PASS original J2D placement, scissor, viewport, projection and RGBA order\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
