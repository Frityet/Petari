#pragma once

class LiveActor;

namespace smgpc::compat {

    // Releases the compatibility ownership acquired by
    // MR::tryStartDemoMarioPuppetable. Opening-demo teardown forces gameplay
    // control on; ordinary demo teardown restores the pre-demo state.
    void release_puppetable_demo_control(bool force_enable);

    // Active demos are owned by the actor that started them. Actor/scene
    // teardown uses this to prevent process-global demo state from leaking
    // into the next scene.
    void release_active_demo_for_owner(const LiveActor *owner);

}  // namespace smgpc::compat
