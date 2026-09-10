#include "compat/GameDataSession.hpp"
#include "compat/GameDataOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"
#include "Game/System/ConfigDataHolder.hpp"
#include "Game/System/UserFile.hpp"

#include <aurora/exception.hpp>
#include <array>
#include <cstdio>
#include <stdexcept>

namespace smgpc::compat {

struct GameDataSession::Storage {
    std::shared_ptr<runtime::ScenarioCatalogOwnership> catalog;
    std::shared_ptr<JkrAllocationDomain> domain;
    UserFile* current = nullptr;
    UserFile* backup = nullptr;

    ~Storage() {
        JkrHostAllocationScope host;
        for (auto* file : {backup, current}) {
            if (!file) continue;
            game_data::destroy_holder(*file->mGameDataHolder);
            delete file->mGameDataHolder;
            // Config children contain only raw scalars/pointers and have no
            // nontrivial destructor. Their allocations share this solid heap.
            delete file->mConfigDataHolder;
            delete[] file->mUserName;
            delete file;
        }
        domain.reset();
        catalog.reset();
    }
};

GameDataSession::GameDataSession(u16 selected_file, const resource::GameResourceRuntime& resources,
                                 std::shared_ptr<runtime::ScenarioCatalogOwnership> catalog)
    : _selected_file(selected_file) {
    JkrHostAllocationScope host;
    if (selected_file < 1 || selected_file > 6)
        aurora::throw_host_exception<std::out_of_range>("Selected game-data file is outside [1, 6]");
    if (!catalog || runtime::ScenarioCatalogOwnership::active() != catalog.get())
        aurora::throw_host_exception<std::logic_error>("Original user files require the active process scenario catalog");

    game_data::initialize_event_table();
    auto storage = std::make_unique<Storage>();
    storage->catalog = std::move(catalog);
    storage->domain = JkrAllocationDomain::create(resources.host_heaps(), resources.budget().save_data_bytes);
    {
        JkrAllocationScope heap(storage->domain);
        storage->current = new UserFile();
        storage->backup = new UserFile();
    }
    char name[16];
    std::snprintf(name, sizeof(name), "mario%u", static_cast<unsigned>(selected_file));
    std::snprintf(storage->current->mGameDataHolder->mName, sizeof(storage->current->mGameDataHolder->mName), "%s", name);
    std::snprintf(storage->backup->mGameDataHolder->mName, sizeof(storage->backup->mGameDataHolder->mName), "%s", name);
    _storage = std::move(storage);
    {
        JkrAllocationScope heap(_storage->domain);
        std::array<u8, 256> config{};
        const auto size = _storage->current->mConfigDataHolder->makeFileBinary(config.data(), config.size());
        std::snprintf(name, sizeof(name), "config%u", static_cast<unsigned>(selected_file));
        _storage->current->loadFromConfigDataBinary(name, config.data(), size);
        if (_storage->current->mIsConfigDataCorrupted)
            aurora::throw_host_exception<std::runtime_error>("The original selected-file config failed its format validation");
    }
    store_scene_start();
    _override.emplace(holder(), _storage->backup->mGameDataHolder);
}

GameDataSession::~GameDataSession() {
    _override.reset();
    _storage.reset();
}

u16 GameDataSession::selected_file() const { return _selected_file; }
GameDataHolder& GameDataSession::holder() { return *_storage->current->mGameDataHolder; }
const GameDataHolder& GameDataSession::holder() const { return *_storage->current->mGameDataHolder; }
UserFile& GameDataSession::user_file() { return *_storage->current; }
const GameDataHolder& GameDataSession::scene_start_holder() const { return *_storage->backup->mGameDataHolder; }

void GameDataSession::store_scene_start() {
    JkrAllocationScope heap(_storage->domain);
    std::array<u8, 4096> game{};
    std::array<u8, 256> config{};
    const auto game_size = holder().makeFileBinary(game.data(), game.size());
    const auto config_size = _storage->current->mConfigDataHolder->makeFileBinary(config.data(), config.size());
    _storage->backup->loadFromGameDataBinary(holder().mName, game.data(), game_size);
    _storage->backup->loadFromConfigDataBinary(_storage->current->getConfigDataName(), config.data(), config_size);
    if (_storage->backup->mIsGameDataCorrupted || _storage->backup->mIsConfigDataCorrupted)
        aurora::throw_host_exception<std::runtime_error>("The original selected-file snapshot failed its format validation");
}
} // namespace smgpc::compat
