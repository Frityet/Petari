#pragma once

#include "Game/System/ConfigDataHolder.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace smgpc::test {
inline SaveDataHandleSequence& original_save_sequence() {
    auto* system = SingletonHolder<GameSystem>::get();
    if (!system || !system->mSequenceDirector || !system->mSequenceDirector->mSaveDataHandleSequence)
        throw std::logic_error("Save assertions require the initialized original process");
    return *system->mSequenceDirector->mSaveDataHandleSequence;
}

// Preserve the actual selected file while an original-process observer exercises
// its public APIs. No holder or sequence pointer is replaced or globally bound.
class OriginalUserFileSnapshot final {
public:
    explicit OriginalUserFileSnapshot(UserFile& file)
        : _file(file), _game_name(file.getGameDataName()), _config_name(file.getConfigDataName()),
          _mario(file.mIsPlayerMario), _game_corrupt(file.mIsGameDataCorrupted),
          _config_corrupt(file.mIsConfigDataCorrupted), _player_status(*file.mGameDataHolder->mPlayerStatus) {
        _game_size = file.mGameDataHolder->makeFileBinary(_game.data(), _game.size());
        _config_size = file.mConfigDataHolder->makeFileBinary(_config.data(), _config.size());
        std::copy_n(file.mUserName, _name.size(), _name.begin());
    }
    ~OriginalUserFileSnapshot() {
        _file.loadFromGameDataBinary(_game_name.c_str(), _game.data(), _game_size);
        _file.loadFromConfigDataBinary(_config_name.c_str(), _config.data(), _config_size);
        *_file.mGameDataHolder->mPlayerStatus = _player_status;
        _file.setUserName(_name.data());
        _file.mIsPlayerMario = _mario;
        _file.mIsGameDataCorrupted = _game_corrupt;
        _file.mIsConfigDataCorrupted = _config_corrupt;
    }
    OriginalUserFileSnapshot(const OriginalUserFileSnapshot&) = delete;
    OriginalUserFileSnapshot& operator=(const OriginalUserFileSnapshot&) = delete;
private:
    UserFile& _file;
    std::string _game_name, _config_name;
    std::array<u8, 4096> _game{};
    std::array<u8, 256> _config{};
    std::array<wchar_t, 11> _name{};
    s32 _game_size = 0, _config_size = 0;
    bool _mario, _game_corrupt, _config_corrupt;
    GameDataPlayerStatus _player_status;
};
}
