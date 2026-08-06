#pragma once

namespace smgpc::runtime {
    class PlayerSystemService;
}

namespace smgpc::compat {

    [[nodiscard]] smgpc::runtime::PlayerSystemService *active_player_system_for_player_util();

    // Restores story-backed player permissions at the same stage-start
    // boundary used by the original GameSequenceProgress.
    void restore_stage_player_permissions(smgpc::runtime::PlayerSystemService &service);

    // Provides the same player boundary to headless compatibility tools and
    // focused tests that RuntimeContext provides to the running game.
    class ScopedPlayerSystemServiceOverride final {
    public:
        explicit ScopedPlayerSystemServiceOverride(smgpc::runtime::PlayerSystemService &service);
        ~ScopedPlayerSystemServiceOverride();

        ScopedPlayerSystemServiceOverride(const ScopedPlayerSystemServiceOverride &) = delete;
        ScopedPlayerSystemServiceOverride &operator=(const ScopedPlayerSystemServiceOverride &) = delete;

    private:
        smgpc::runtime::PlayerSystemService *_previous = nullptr;
    };

}  // namespace smgpc::compat
