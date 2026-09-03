#include "compat/CameraViewRuntime.hpp"

namespace smgpc::compat {
    namespace {
        thread_local CameraViewOutput *sOutput = nullptr;
    }

    CameraViewOutput::CameraViewOutput() {
        view.identity();
        inverse_view.identity();
    }

    ScopedCameraViewOutput::ScopedCameraViewOutput(CameraViewOutput &output) : _previous(sOutput) {
        sOutput = &output;
    }

    ScopedCameraViewOutput::~ScopedCameraViewOutput() {
        sOutput = _previous;
    }

    CameraViewOutput *bound_camera_view_output() noexcept {
        return sOutput;
    }
}  // namespace smgpc::compat
