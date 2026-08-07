#pragma once

#include <string_view>

class LiveActor;
class NameObj;

namespace smgpc::compat {

    // Releases the compatibility ownership acquired by
    // MR::tryStartDemoMarioPuppetable. Opening-demo teardown forces gameplay
    // control on; ordinary demo teardown restores the pre-demo state.
    void release_puppetable_demo_control(bool force_enable);

    // Scene time-keep demos and programmable demos share the original single
    // DemoDirector-active state. The scene clock uses these hooks without
    // putting host ownership into Game-side source.
    void activate_demo_state(const NameObj *owner, std::string_view demo_name,
                             bool puppetable);
    void finish_demo_state(const NameObj *owner, std::string_view demo_name);

    // Active demos are owned by the actor that started them. Actor/scene
    // teardown uses this to prevent process-global demo state from leaking
    // into the next scene.
    void release_active_demo_for_owner(const LiveActor *owner);

}  // namespace smgpc::compat
