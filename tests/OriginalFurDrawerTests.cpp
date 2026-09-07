#include "Game/Util/FurDrawer.hpp"
#include "Game/Util/FurParam.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "render/RendererService.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    struct alignas(32) TextureResource {
        ResTIMG header;
        std::array<u8, 32> image{};

        TextureResource() {
            header.mFormat = GX_TF_I8;
            header.mWidth = 8;
            header.mHeight = 4;
            image.fill(255);
        }
    };

    void check_procedural_words(FurDrawer& drawer) {
        auto* texture = drawer.mFurTexture;
        require(texture->getWidth() == 32 && texture->getHeight() == 32 && texture->getFormat() == GX_TF_IA8,
                "original constructor must create the authored 32 by 32 IA8 map");
        require(texture->mImage == reinterpret_cast<const u8*>(texture->mTIMG) + sizeof(ResTIMG),
                "procedural texels must remain in the actual aligned resource allocation");
        require((reinterpret_cast<std::uintptr_t>(texture->mImage) & 31U) == 0U,
                "original aligned texture allocation must remain aligned on the host");
        std::fill(std::begin(drawer.mDensity), std::end(drawer.mDensity), 0.0F);
        drawer.createFurMap();
        for (std::size_t texel = 0; texel < 1024; ++texel) {
            require(texture->mImage[texel * 2] == 0 && texture->mImage[texel * 2 + 1] == 255,
                    "an empty procedural map must have the original 00 FF byte pairs");
        }
        for (std::size_t pass = 0; pass < 4; ++pass) {
            std::fill(std::begin(drawer.mDensity), std::end(drawer.mDensity), 0.0F);
            drawer.mDensity[pass] = 1.0F / 1024.0F;
            drawer.mIntensity[pass] = 0.5F;
            drawer.mTransparency[pass] = 73;
            drawer.createFurMap();
            std::size_t marked = 0;
            for (std::size_t texel = 0; texel < 1024; ++texel) {
                const auto first = texture->mImage[texel * 2];
                const auto second = texture->mImage[texel * 2 + 1];
                if (first != 0 || second != 255) {
                    require(first == 127 && second == 182,
                            "original halfword stores must retain Wii byte order and truncation");
                    ++marked;
                }
            }
            require(marked == 1, "each selected density pass must place exactly one original random texel");
        }
    }

    void check_layer_interpolation() {
        const FurDrawer::CLayerParam linear{10.0F, 2.0F, 1.0F};
        const FurDrawer::CLayerParam quadratic{10.0F, 2.0F, 2.0F};
        require(linear.calcValue(0, 4) == 4.0F && linear.calcValue(3, 4) == 10.0F,
                "layer interpolation must include the first positive step and final endpoint");
        require(quadratic.calcValue(1, 4) == 4.0F && quadratic.calcValue(3, 4) == 10.0F,
                "layer exponent must affect distance from the original start value");
    }
}

int main() {
    try {
        smgpc::render::AuroraWindow window({.width = 320, .height = 240, .title = "Original fur texture ownership"});
        smgpc::render::AuroraRenderer renderer(window);
        OSInit();
        auto runtime = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
        auto domain = smgpc::compat::JkrAllocationDomain::create(runtime, 256U * 1024U);
        TextureResource base;
        TextureResource indirect;
        (void)renderer.begin_frame();
        {
            smgpc::compat::JkrAllocationScope allocation(domain);
            auto* drawer = new FurDrawer(6, &base.header, &indirect.header);
            require(drawer->mBaseTexture->mTIMG == &base.header && drawer->mLayerCount == 6,
                    "constructor must retain actual base texture and requested layer count");
            require(drawer->mIndirectTexture == nullptr && drawer->mUseIndirect == 0,
                    "constructor must preserve its retail member test even when an indirect image is supplied");
            check_procedural_words(*drawer);
            check_layer_interpolation();
            drawer->mFurScale = 2.5F;
            drawer->update();
            require(drawer->mFurMtx[0][0] == 2.5F && drawer->mFurMtx[1][1] == 2.5F && drawer->mFurMtx[2][2] == 1.0F,
                    "original update scales the two texture coordinates independently from homogeneous depth");
            drawer->mBrightness.mStart = 0.0F;
            drawer->mAlpha.mStart = 0.0F;
            drawer->mOffset.mStart = 0.0F;
            drawer->mIndirect.mStart = 0.0F;
            DynamicFurParam dynamic{nullptr, nullptr};
            drawer->setupMaterial(&dynamic);
            drawer->setupLayerMaterial(0);
            FurLightParam light;
            light.mLight1Enabled = 2;
            dynamic.mLightParam = &light;
            drawer->mIndirectTexture = new JUTTexture(&indirect.header, 0);
            drawer->mUseIndirect = 1;
            drawer->setupMaterial(&dynamic);
            drawer->setupLayerMaterial(5);
        }
        renderer.end_frame();
        domain.reset();
        std::cout << "original fur constructor, four procedural passes, layer math, GX material commands and heap retirement passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "original fur drawer tests failed: " << error.what() << '\n';
        return 1;
    }
}
