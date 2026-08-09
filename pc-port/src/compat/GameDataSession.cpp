#include "compat/GameDataSession.hpp"

#include <cstdio>
#include <stdexcept>

#include "compat/GameDataHolderCompat.hpp"

namespace smgpc::compat {

GameDataSession::GameDataSession(u16 selected_file)
    : _selected_file(selected_file), _holder(nullptr) {
    try {
        if (_selected_file < 1U || _selected_file > 6U) {
            throw std::out_of_range("Selected game-data file is outside [1, 6]");
        }

        char name[sizeof(_holder.mName)];
        std::snprintf(name, sizeof(name), "mario%u", static_cast<unsigned>(_selected_file));
        game_data::set_holder_name(_holder, name);
        _holder.followStoryEventByName("ピーチ城浮上後");
        _override.emplace(_holder, &_holder);
    } catch (...) {
        game_data::destroy_holder_state(_holder);
        throw;
    }
}

GameDataSession::~GameDataSession() {
    _override.reset();
    game_data::destroy_holder_state(_holder);
}

u16 GameDataSession::selected_file() const {
    return _selected_file;
}

GameDataHolder& GameDataSession::holder() {
    return _holder;
}

const GameDataHolder& GameDataSession::holder() const {
    return _holder;
}

}  // namespace smgpc::compat
