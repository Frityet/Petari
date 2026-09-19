#pragma once

class LiveActor;

namespace smgpc::compat {
    // Applies the host-side equivalent of LiveActor::updateBinder(). Actors
    // without a binder take the original free-motion path; configured binders
    // use the active KCL-backed stage collision service when one is present.
    void integrate_live_actor_velocity(LiveActor &actor);
}  // namespace smgpc::compat
