#pragma once

class Binder;
class LiveActor;

namespace smgpc::compat {
    // Associates an exact Binder provider with the generalized actor state
    // needed to remove host-only render scale from its base-matrix basis.
    void register_binder_owner(Binder* binder, const LiveActor* actor);
    void release_binder_owner(const Binder* binder) noexcept;
}  // namespace smgpc::compat
