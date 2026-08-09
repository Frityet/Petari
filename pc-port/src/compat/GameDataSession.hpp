#pragma once

#include "Game/System/GameDataHolder.hpp"
#include "compat/GameDataFunctionCompat.hpp"

#include <optional>

#include <revolution/types.h>

namespace smgpc::compat {

class GameDataSession final {
public:
    explicit GameDataSession(u16 selected_file);
    ~GameDataSession();

    GameDataSession(const GameDataSession&) = delete;
    GameDataSession& operator=(const GameDataSession&) = delete;
    GameDataSession(GameDataSession&&) = delete;
    GameDataSession& operator=(GameDataSession&&) = delete;

    [[nodiscard]] u16 selected_file() const;
    [[nodiscard]] GameDataHolder& holder();
    [[nodiscard]] const GameDataHolder& holder() const;

private:
    u16 _selected_file = 0U;
    GameDataHolder _holder;
    std::optional<ScopedGameDataHolderOverride> _override;
};

}  // namespace smgpc::compat
