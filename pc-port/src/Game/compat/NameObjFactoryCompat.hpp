#pragma once

#include <memory>
#include <string_view>

class NameObj;

namespace smgpc::game {

    [[nodiscard]] bool can_create_name_obj(std::string_view object_name);
    [[nodiscard]] std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name);

}  // namespace smgpc::game
