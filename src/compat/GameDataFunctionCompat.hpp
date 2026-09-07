#pragma once

class GameDataHolder;

namespace smgpc::compat {

// Binds an explicitly owned retail GameDataHolder to the original
// GameDataFunction entry points. This is intentionally narrower than a fake
// SaveDataHandleSequence: persistence/NAND remain absent, while authored
// scene checkpoints can exercise the real story-event code against a real
// holder.
class ScopedGameDataHolderOverride final {
public:
    explicit ScopedGameDataHolderOverride(GameDataHolder& current,
                                          GameDataHolder* scene_start = nullptr);
    ~ScopedGameDataHolderOverride();

    ScopedGameDataHolderOverride(const ScopedGameDataHolderOverride&) = delete;
    ScopedGameDataHolderOverride& operator=(const ScopedGameDataHolderOverride&) = delete;

private:
    GameDataHolder* _previous_current = nullptr;
    GameDataHolder* _previous_scene_start = nullptr;
};

}  // namespace smgpc::compat
