#include "Game/Util/CameraUtil.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/SystemConfigService.hpp"
#include <aurora/aurora.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error& error) {
            require(std::string_view(error.what()).find("SceneObj 23") != std::string_view::npos,
                    "debug failure must identify the missing original CameraContext");
            unavailable = true;
        }
        require(unavailable, message);
    }

    bool same(f32 lhs, f32 rhs) {
        return std::abs(lhs - rhs) <= 0.00001F;
    }
}

int main() {
#if defined(NDEBUG)
    std::cout << "Camera missing-owner diagnostics require a debug build; release retains original preconditions\n";
    return 0;
#else
    aurora::g_config.mem1Size = 24U << 20;
    smgpc::resource::GameResourceRuntime resources;
    aurora::NandFileSystem nand;
    smgpc::runtime::SystemConfigService settings(nand);
    auto passed = 0;

    require_unavailable([] { (void)MR::getCamPos(); }, "missing camera state must not manufacture an origin");
    require_unavailable([] { (void)MR::getCamXdir(); }, "missing camera state must not manufacture an X axis");
    require_unavailable([] { (void)MR::getCamYdir(); }, "missing camera state must not manufacture a Y axis");
    require_unavailable([] { (void)MR::getCamZdir(); }, "missing camera state must not manufacture a Z axis");
    require_unavailable([] { (void)MR::getAspect(); }, "missing camera state must not manufacture an aspect ratio");
    require_unavailable([] { (void)MR::getNearZ(); }, "missing camera state must not manufacture a near clip");
    require_unavailable([] { (void)MR::getFarZ(); }, "missing camera state must not manufacture a far clip");
    require_unavailable([] { (void)MR::getFovy(); }, "missing camera state must not manufacture a field of view");
    require_unavailable([] { (void)MR::calcCameraDistanceZ(TVec3f{1.0F, 2.0F, 3.0F}); },
                        "missing camera state must not manufacture a camera-space distance");
    ++passed;

    auto screen2 = TVec2f{12.0F, 34.0F};
    require_unavailable([&] { (void)MR::calcScreenPosition(&screen2, TVec3f{1.0F, 2.0F, 3.0F}); },
                        "projection requires the original CameraContext");
    require(same(screen2.x, 12.0F) && same(screen2.y, 34.0F), "absent projection must not write synthetic screen coordinates");

    auto screen3 = TVec3f{12.0F, 34.0F, 56.0F};
    require_unavailable([&] { (void)MR::calcScreenPosition(&screen3, TVec3f{1.0F, 2.0F, 3.0F}); },
                        "3D projection requires the original CameraContext");
    require(same(screen3.x, 12.0F) && same(screen3.y, 34.0F) && same(screen3.z, 56.0F),
            "absent 3D projection must not write synthetic coordinates");
    ++passed;

    auto world = TVec3f{7.0F, 8.0F, 9.0F};
    require_unavailable([&] { (void)MR::calcWorldPositionFromScreen(&world, TVec2f{100.0F, 200.0F}, 500.0F); },
                        "unprojection requires the original CameraContext");
    require(same(world.x, 7.0F) && same(world.y, 8.0F) && same(world.z, 9.0F),
            "absent unprojection must not write a synthetic world position");
    require_unavailable([&] { (void)MR::calcWorldRayDirectionFromScreen(&world, TVec2f{100.0F, 200.0F}); },
                        "world-ray calculation requires the original CameraContext");
    require(same(world.x, 7.0F) && same(world.y, 8.0F) && same(world.z, 9.0F),
            "absent world-ray calculation must preserve the caller's value");
    ++passed;

    std::cout << "Camera real-or-absent tests passed: " << passed << "/3\n";
    return 0;
#endif
}
