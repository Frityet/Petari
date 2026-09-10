#include "Game/Camera/CameraContext.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
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
    void check_original_utilities(CameraContext& camera) {
        const float width = MR::getScreenWidth();
        const float height = MR::getScreenHeight();
        const float aspect = camera.getAspect();
        require(&MR::getCameraViewMtx() == &camera.mView &&
                    &MR::getCameraInvViewMtx() == &camera.mViewInv &&
                    &MR::getCameraProjectionMtx() == &camera.mProjection,
                "original MR utility references belong to the actual scene CameraContext without RuntimeContext pose publication");
        TPos3f view;
        view.identity();
        MR::setCameraViewMtx(view, true, true, TVec3f{});
        MR::setNearZ(8.0f);
        camera.setFarZ(1024.0f);
        MR::setFovy(90.0f);
        MR::setShakeOffset(0.0f, 0.0f);
        require(MR::getNearZ() == 8.0f && MR::getFarZ() == 1024.0f && MR::getFovy() == 90.0f &&
                    MR::getAspect() == aspect, "original scalar helpers read and mutate the actual context");
        const TVec3f near_point(2.0f * aspect, 4.0f, -8.0f);
        TVec3f normalized, screen3;
        TVec2f screen2;
        require(MR::calcNormalizedScreenPositionFromView(&normalized, near_point), "negative view Z is in front");
        close(normalized.x, 0.25f, "original homogeneous X divide");
        close(normalized.y, -0.5f, "original projected Y is inverted");
        close(normalized.z, -1.0f, "original projected depth at near plane");
        require(MR::calcScreenPosition(&screen2, near_point) && MR::calcScreenPosition(&screen3, near_point),
                "both original screen overloads preserve visibility");
        close(screen2.x / width, 0.625f, "screen X uses actual console width");
        close(screen2.y / height, 0.25f, "screen Y uses actual console height");
        require(screen2.x == screen3.x && screen2.y == screen3.y, "screen overloads share original conversion");
        close(screen3.z, -1.0f, "three-component screen result retains projected depth");
        require(!MR::calcScreenPosition(&screen3, TVec3f(9.0f * aspect, 0.0f, -8.0f)),
                "screen projection must reject points outside the horizontal clip interval");
        require(!MR::calcNormalizedScreenPositionFromView(&normalized, TVec3f(0.0f, 9.0f, -8.0f)),
                "view projection must reject points outside the vertical clip interval");
        require(MR::calcNormalizedScreenPositionFromView(&normalized, TVec3f(0.0f, 0.0f, -4.0f)) && normalized.z < -1.0f,
                "retail visibility has no lower depth test before the near plane");
        require(MR::calcNormalizedScreenPositionFromView(&normalized, TVec3f(0.0f, 0.0f, -1024.0f)),
                "the original far plane remains visible");
        close(normalized.z, 0.0f, "original projected depth at far plane");
        require(!MR::calcNormalizedScreenPositionFromView(&normalized, TVec3f(0.0f, 0.0f, -2048.0f)) && normalized.z > 0.0f,
                "positive projected depth beyond the far plane is rejected");
        require(!MR::calcNormalizedScreenPositionFromView(&normalized, TVec3f(0.0f, 0.0f, 8.0f)),
                "a point behind the camera is rejected by the original depth predicate");
        MR::setShakeOffset(0.125f, -0.25f);
        require(MR::calcNormalizedScreenPosition(&normalized, near_point), "original world projection uses current shake");
        close(normalized.x, 0.375f, "shake X comes from actual projection matrix");
        close(normalized.y, -0.25f, "shake Y precedes original screen inversion");
        camera.mProjection.mMtx[0][0] *= 2.0f;
        require(MR::calcNormalizedScreenPosition(&normalized, near_point), "direct context projection edits are immediately visible");
        close(normalized.x, 0.625f, "projection is never rebuilt from cached pose or FOV during a query");
        MR::setShakeOffset(0.0f, 0.0f);

        view.mMtx[0][3] = -32.0f;
        view.mMtx[1][3] = 16.0f;
        view.mMtx[2][3] = -64.0f;
        MR::setCameraViewMtx(view, false, false, TVec3f{});
        require(MR::getCamPos().epsilonEquals(TVec3f(32.0f, -16.0f, 64.0f), 0.00001f) &&
                    MR::getCamXdir().epsilonEquals(TVec3f(1.0f, 0.0f, 0.0f), 0.00001f) &&
                    MR::getCamYdir().epsilonEquals(TVec3f(0.0f, 1.0f, 0.0f), 0.00001f) &&
                    MR::getCamZdir().epsilonEquals(TVec3f(0.0f, 0.0f, -1.0f), 0.00001f),
                "original camera position and basis come from the current inverse view");
        close(MR::calcCameraDistanceZ(TVec3f(32.0f, -16.0f, -36.0f)), 100.0f, "distance reads current view translation");
        close(MR::calcCameraDistanceZ(TVec3f(32.0f, -16.0f, 164.0f)), 100.0f, "distance uses original absolute view Z");
        MR::loadViewMtx();
        require(j3dSys.mViewMtx[0][3] == -32.0f && j3dSys.mViewMtx[1][3] == 16.0f && j3dSys.mViewMtx[2][3] == -64.0f,
                "original loadViewMtx writes the actual J3D system view");

        const TVec2f centered(57.0f, -28.5f);
        const TVec2f pixel(width * 0.5f + centered.x, height * 0.5f + centered.y);
        TVec3f from_pixel, from_center, ray;
        require(MR::calcWorldPositionFromScreen(&from_pixel, pixel, 456.0f) &&
                    MR::calcWorldPositionFromCenterScreen(&from_center, centered, 456.0f),
                "both original unprojection APIs accept explicit camera distance");
        require(from_pixel.epsilonEquals(TVec3f(146.0f, 41.0f, -392.0f), 0.0001f) &&
                    from_center.epsilonEquals(from_pixel, 0.00001f),
                "original unprojection uses screen-height focal length and current inverse view");
        require(MR::calcScreenPosition(&screen2, from_pixel), "unprojected point projects through current view");
        close((screen2.x - width * 0.5f) / centered.x, width / (height * aspect),
              "round-trip X preserves original wide-screen dimension convention");
        close(screen2.y, pixel.y, "original unprojection round-trip Y");
        if (width == 608.0f) close(screen2.x, pixel.x, "four-by-three round-trip X");
        require(MR::calcWorldRayDirectionFromScreen(&ray, pixel) &&
                    ray.epsilonEquals(TVec3f(57.0f, 28.5f, -228.0f), 0.0001f),
                "negative distance selects focal length and the original ray remains unnormalized");
        require(MR::calcWorldPositionFromCenterScreen(nullptr, centered, -7.0f),
                "original centered unprojection permits a null output after calculating its position");
        MR::setShakeOffset(0.125f, -0.25f);
        require(MR::calcWorldPositionFromScreen(&from_center, pixel, 456.0f) &&
                    from_center.epsilonEquals(from_pixel, 0.00001f),
                "retail unprojection intentionally ignores projection shake instead of inventing a matrix inverse");
        MR::setShakeOffset(0.0f, 0.0f);
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
            SceneObjHolder holder;
            smgpc::scene::SceneObjHolderBinding scene(holder, nullptr, nullptr, process.create_cohort());
            auto& camera = *static_cast<CameraContext*>(holder.create(SceneObj_CameraContext));
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
            check_original_utilities(camera);
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
        std::cout << "PASS original CameraContext and CameraUtil: SC aspect, actual matrix identity, projection, unprojection, clipping, shake, rays, distance and J3D view\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
