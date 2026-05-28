#include "Game/compat/NameObjFactoryCompat.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"

#include <stdexcept>
#include <string>

namespace smgpc::game {
    bool can_create_name_obj(std::string_view object_name) {
        const auto name = std::string(object_name);
        return NameObjFactory::canCreate(name.c_str());
    }

    std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name) {
        const auto object = std::string(object_name);
        const auto creator = NameObjFactory::getCreator(object.c_str());
        if (creator == nullptr) {
            throw std::runtime_error("Unsupported NameObj factory request: " + std::string(object_name));
        }

        const auto name = std::string(actor_name);
        return std::unique_ptr<NameObj>(creator(name.c_str()));
    }

}  // namespace smgpc::game
