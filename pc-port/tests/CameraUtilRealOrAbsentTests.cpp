#include "Game/Util/CameraUtil.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>

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
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    bool same(f32 lhs, f32 rhs) {
        return std::abs(lhs - rhs) <= 0.00001F;
    }
}

int main() {
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
    require(!MR::calcScreenPosition(&screen2, TVec3f{1.0F, 2.0F, 3.0F}), "projection must report absent without a real camera");
    require(same(screen2.x, 12.0F) && same(screen2.y, 34.0F), "absent projection must not write synthetic screen coordinates");

    auto screen3 = TVec3f{12.0F, 34.0F, 56.0F};
    require(!MR::calcScreenPosition(&screen3, TVec3f{1.0F, 2.0F, 3.0F}), "3D projection must report absent without a real camera");
    require(same(screen3.x, 12.0F) && same(screen3.y, 34.0F) && same(screen3.z, 56.0F),
            "absent 3D projection must not write synthetic coordinates");
    ++passed;

    auto world = TVec3f{7.0F, 8.0F, 9.0F};
    require(!MR::calcWorldPositionFromScreen(&world, TVec2f{100.0F, 200.0F}, 500.0F),
            "unprojection must report absent without a real camera");
    require(same(world.x, 7.0F) && same(world.y, 8.0F) && same(world.z, 9.0F),
            "absent unprojection must not write a synthetic world position");
    require(!MR::calcWorldRayDirectionFromScreen(&world, TVec2f{100.0F, 200.0F}),
            "world-ray calculation must report absent without a real camera");
    require(same(world.x, 7.0F) && same(world.y, 8.0F) && same(world.z, 9.0F),
            "absent world-ray calculation must preserve the caller's value");
    ++passed;

    std::cout << "Camera real-or-absent tests passed: " << passed << "/3\n";
    return 0;
}
