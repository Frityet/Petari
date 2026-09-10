#pragma once

#include "Game/System/GameDataHolder.hpp"
#include "compat/GameDataFunctionCompat.hpp"

#include <optional>
#include <memory>

#include <revolution/types.h>

class UserFile;
namespace smgpc::resource { class GameResourceRuntime; }
namespace smgpc::runtime { class ScenarioCatalogOwnership; }

namespace smgpc::compat {

class GameDataSession final {
public:
    GameDataSession(u16 selected_file, const resource::GameResourceRuntime& resources,
                    std::shared_ptr<runtime::ScenarioCatalogOwnership> catalog);
    ~GameDataSession();

    GameDataSession(const GameDataSession&) = delete;
    GameDataSession& operator=(const GameDataSession&) = delete;
    GameDataSession(GameDataSession&&) = delete;
    GameDataSession& operator=(GameDataSession&&) = delete;

    [[nodiscard]] u16 selected_file() const;
    [[nodiscard]] GameDataHolder& holder();
    [[nodiscard]] const GameDataHolder& holder() const;
    [[nodiscard]] UserFile& user_file();
    [[nodiscard]] const GameDataHolder& scene_start_holder() const;
    void store_scene_start();

private:
    u16 _selected_file = 0U;
    struct Storage;
    std::unique_ptr<Storage> _storage;
    std::optional<ScopedGameDataHolderOverride> _override;
};

}  // namespace smgpc::compat
