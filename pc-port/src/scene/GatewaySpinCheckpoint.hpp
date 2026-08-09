#pragma once

#include "Game/NameObj/NameObj.hpp"

#include <cstdint>
#include <memory>
#include <span>

class GameDataHolder;
class LiveActor;

namespace smgpc::compat {
    class DemoSceneRuntime;
}

namespace smgpc::runtime {
    class DvdFileSystemService;
    class PlayerSystemService;
    class WipeService;
}

namespace smgpc::scene {
    struct StageGeneralPos;
    struct StagePlacementObject;

    enum class GatewaySpinCheckpointState : std::uint8_t {
        AwaitingRosetta,
        FadeHandoff,
        SpinDemo,
        PromptDelegated,
    };

    struct GatewaySpinCheckpointEvidence {
        std::uint32_t fade_handoff_frames = 0U;
        std::uint32_t pre_prompt_demo_ticks = 0U;
        std::uint32_t prompt_delegate_calls = 0U;
        bool player_row_dispatched_to_mario_demo_pos4 = false;
    };

    // A deliberately bounded post-high-tower route. It starts at the authored
    // story-progress-10 checkpoint and stops at the spin explanation handoff;
    // rabbit pursuit and the remainder of the prologue are outside this owner.
    //
    // The exact InformationObserver, not this controller, owns pausing and
    // resuming the time keeper, displaying the prompt, accepting A, and
    // granting the spin event flag. The caller must pre-attach Mario with its
    // real PlayerActorEntitlementBridge so that grant reaches MarioActor.
    class GatewaySpinCheckpoint final : public NameObj {
    public:
        GatewaySpinCheckpoint(
            smgpc::runtime::DvdFileSystemService &dvd,
            std::span<const StagePlacementObject> placements,
            std::span<const StageGeneralPos> general_positions,
            smgpc::runtime::PlayerSystemService &player,
            smgpc::runtime::WipeService &wipe, LiveActor &mario);
        ~GatewaySpinCheckpoint() override;

        GatewaySpinCheckpoint(const GatewaySpinCheckpoint &) = delete;
        GatewaySpinCheckpoint &operator=(const GatewaySpinCheckpoint &) = delete;

        // Connected at the original NPC movement slot, after DemoDirector.
        void movement() override;

        [[nodiscard]] GatewaySpinCheckpointState state() const;
        [[nodiscard]] const GatewaySpinCheckpointEvidence &evidence() const;
        [[nodiscard]] const LiveActor &tico_cast() const;
        [[nodiscard]] LiveActor &tico_cast();
        [[nodiscard]] const smgpc::compat::DemoSceneRuntime &demo_runtime() const;
        [[nodiscard]] smgpc::compat::DemoSceneRuntime &demo_runtime();
        [[nodiscard]] const GameDataHolder &checkpoint_game_data() const;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::scene
