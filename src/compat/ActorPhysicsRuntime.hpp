#pragma once

class LiveActor;
namespace smgpc::camera {
    struct CameraPose;
}

namespace smgpc::compat {
    void update_actor_clipping(LiveActor& actor, const smgpc::camera::CameraPose& camera);
}  // namespace smgpc::compat
