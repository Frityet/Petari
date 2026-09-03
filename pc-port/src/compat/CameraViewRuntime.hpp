#pragma once

#include <JSystem/JGeometry/TMatrix.hpp>

namespace smgpc::compat {

    // The view/projection values owned by the scene's camera context. An
    // original Game calculation binds its destination for the MR setters.
    struct CameraViewOutput {
        CameraViewOutput();
        TPos3f view;
        TPos3f inverse_view;
        float fovy = 45.0F;
    };

    class ScopedCameraViewOutput final {
    public:
        explicit ScopedCameraViewOutput(CameraViewOutput &output);
        ~ScopedCameraViewOutput();
        ScopedCameraViewOutput(const ScopedCameraViewOutput &) = delete;
        ScopedCameraViewOutput &operator=(const ScopedCameraViewOutput &) = delete;

    private:
        CameraViewOutput *_previous;
    };

    [[nodiscard]] CameraViewOutput *bound_camera_view_output() noexcept;

}  // namespace smgpc::compat
