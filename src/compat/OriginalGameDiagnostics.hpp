#pragma once

class NameObj;

namespace smgpc::compat {
    NameObj* require_scene_object_for_debug(int id);
    void validate_player_owner_for_debug();
}
