#pragma once

class MarioActor;

namespace smgpc::runtime {
    class DvdFileSystemService;
}
namespace smgpc::compat {
    class DemoSceneRuntime;
}
namespace smgpc::tests {
    // Uses the live attached Mario and restores every touched actor/state field.
    // The demo check owns a temporary, cast-free runtime and never advances it.
    void verify_original_mario_camera_target(
        MarioActor& actor, runtime::DvdFileSystemService& dvd,
        const compat::DemoSceneRuntime& scene_demo);
}
