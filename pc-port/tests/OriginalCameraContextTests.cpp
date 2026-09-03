#include "Game/Camera/CameraContext.hpp"
#include "Game/System/RenderMode.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/SystemConfigService.hpp"
#include <aurora/aurora.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    void close(float value, float expected, const char* message) {
        require(std::abs(value - expected) < 0.00002f, message);
    }
    void check_projection(const CameraContext& camera, float half_width, float half_height) {
        const auto& p = camera.mProjection.mMtx;
        // Points on the near-plane edges must project to the NDC square. Wii
        // reversed depth maps that plane to -1 and the far plane to zero.
        close(p[0][0] * half_width / camera.getNearZ(), 1.0f, "horizontal near-plane edge");
        close(p[1][1] * half_height / camera.getNearZ(), 1.0f, "vertical near-plane edge");
        close((-camera.getNearZ() * p[2][2] + p[2][3]) / camera.getNearZ(), -1.0f, "near depth");
        close((-camera.getFarZ() * p[2][2] + p[2][3]) / camera.getFarZ(), 0.0f, "far depth");
        require(p[3][2] == -1.0f && p[3][3] == 0.0f, "perspective homogeneous row");
    }
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime process;
        aurora::NandFileSystem nand;
        smgpc::runtime::SystemConfigService settings(nand);
        const auto objects = smgpc::compat::name_obj_runtime_state_count();
        for (u8 flag : {0, 1, 2, 255}) {
            require(SCReplaceU8Item(flag, SC_ITEM_ID_IPL_ASPECT_RATIO), "set original SC aspect item");
            const bool wide = flag == 1;
            require(MR::isScreen16Per9() == wide && MR::getScreenWidth() == (wide ? 832 : 608),
                    "original SC accessor controls screen width without a camera pose");
            CameraContext camera;
            const float aspect = wide ? 16.0f / 9.0f : 4.0f / 3.0f;
            require(camera.getAspect() == aspect && camera.getFovy() == 45.0f &&
                        camera.getNearZ() == 100.0f && camera.getFarZ() == 800000.0f,
                    "original camera initialization and aspect selection");
            TVec3f eye(0.0f, 0.0f, 3000.0f), origin;
            camera.getViewMtx().mult(eye, origin);
            close(origin.x, 0.0f, "initial view eye x");
            close(origin.y, 0.0f, "initial view eye y");
            close(origin.z, 0.0f, "initial view eye z");
            camera.getInvViewMtx().mult(origin, eye);
            close(eye.z, 3000.0f, "initial inverse recovers camera position");
            camera.setNearZ(8.0f);
            camera.setFarZ(1024.0f);
            camera.setFovy(90.0f);
            check_projection(camera, 8.0f * aspect, 8.0f);
            camera.setShakeOffset(0.125f, -0.25f);
            close(-camera.mProjection.mMtx[0][2], 0.125f, "shake shifts horizontal normalized coordinate");
            close(-camera.mProjection.mMtx[1][2], -0.25f, "shake shifts vertical normalized coordinate");
            camera.setShakeOffset(0.0f, 0.0f);
            check_projection(camera, 8.0f * aspect, 8.0f);
            TPos3f view;
            view.identity();
            view.mMtx[0][3] = -32.0f;
            view.mMtx[1][3] = 16.0f;
            view.mMtx[2][3] = -64.0f;
            camera.setViewMtx(view, true, true, TVec3f(100.0f, 200.0f, 300.0f));
            TVec3f position(32.0f, -16.0f, 64.0f);
            camera.getViewMtx().mult(position, origin);
            close(origin.x, 0.0f, "set view x");
            close(origin.y, 0.0f, "set view y");
            close(origin.z, 0.0f, "set view z");
            camera.getInvViewMtx().mult(origin, eye);
            require(eye.x == position.x && eye.y == position.y && eye.z == position.z, "set view updates real inverse");
            require(SCReplaceU8Item(wide ? 0 : 1, SC_ITEM_ID_IPL_ASPECT_RATIO), "change live console aspect");
            camera.updateProjectionMtx();
            const auto new_aspect = wide ? 4.0f / 3.0f : 16.0f / 9.0f;
            require(camera.getAspect() == new_aspect, "camera queries current console aspect instead of cached initialization");
            check_projection(camera, 8.0f * new_aspect, 8.0f);
        }
        require(smgpc::compat::name_obj_runtime_state_count() == objects, "original camera NameObj identities retire");
        std::cout << "PASS original CameraContext: SC aspect, view/inverse, clipping, FOV, shake and live aspect updates\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
