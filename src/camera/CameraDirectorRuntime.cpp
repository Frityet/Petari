#include "camera/CameraDirectorRuntime.hpp"

#include "Game/Camera/CameraContext.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/exception.hpp>

#include <stdexcept>

namespace smgpc::camera {
    namespace {
        CameraDirectorRuntime *sCamera = nullptr;
    }

    CameraDirectorRuntime::CameraDirectorRuntime(CameraContext &context, CameraDirector &director)
        : _context(&context), _director(&director) {
        if (sCamera != nullptr)
            aurora::throw_host_exception<std::logic_error>("Original camera ownership requires one bound scene camera");
        sCamera = this;
    }

    CameraDirectorRuntime::~CameraDirectorRuntime() {
        unpublish();
    }

    void CameraDirectorRuntime::unpublish() noexcept {
        if (sCamera == this) sCamera = nullptr;
    }

    void *CameraDirectorRuntime::retain_animation(std::span<const std::uint8_t> resource) {
        compat::JkrHostAllocationScope host;
        auto data = CameraAnimation::from_bytes(resource).native_data();
        _animations.push_back(data);
        return const_cast<std::uint8_t *>(data.bytes().data());
    }

    void CameraDirectorRuntime::close_creating_chunks() {
        if (_ready) {
            aurora::throw_host_exception<std::logic_error>(
                "Original camera chunks have already completed placement");
        }
        if (_director->getTarget() == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Original camera placement requires an actual camera target");
        }
        _director->closeCreatingCameraChunk();
        _ready = true;
    }

    CameraPose CameraDirectorRuntime::pose() const {
        TVec3f eye, up, back;
        _context->mViewInv.getTrans(eye);
        _context->mViewInv.getYDir(up);
        _context->mViewInv.getZDir(back);
        // A native CameraPose stores a point along the viewing direction.
        // The original context's inverse matrix is authoritative, including
        // its interpolation and subjective-camera result.
        const auto watch = eye - back;
        return CameraPose{
            .eye = {eye.x, eye.y, eye.z},
            .watch = {watch.x, watch.y, watch.z},
            .up = {up.x, up.y, up.z},
            .fovy_degrees = _context->mFovy,
            .aspect_ratio = _context->getAspect(),
            .near_clip = _context->mNearZ,
            .far_clip = _context->mFarZ,
            .projection_offset_x = _context->mShakeOffset.x,
            .projection_offset_y = _context->mShakeOffset.y,
        };
    }

    CameraDirectorRuntime *current_camera_director_runtime() noexcept {
        return sCamera;
    }

    CameraContext *current_original_camera_context() noexcept {
        return sCamera ? &sCamera->context() : nullptr;
    }
}
