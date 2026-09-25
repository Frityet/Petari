#include "Game/Util/LiveActorUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    [[nodiscard]] GroupCheckManager &require_active_group_check_manager() {
        auto *holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Attribute groups require an active scene object holder.");
        }

        auto *object = holder->getObj(SceneObj_GroupCheckManager);
        if (object == nullptr) {
            aurora::throw_host_exception<std::logic_error>("The active scene has no pre-created GroupCheckManager.");
        }
        return *static_cast<GroupCheckManager *>(object);
    }

    [[nodiscard]] const NameObj &require_attribute_group_object(const LiveActor *actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Attribute group membership requires a LiveActor.");
        }
        return *actor;
    }

}  // namespace

namespace MR {
    void addToAttributeGroupSearchTurtle(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        require_active_group_check_manager().add(&object, 0);
    }

    void addToAttributeGroupReflectSpinningBox(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        require_active_group_check_manager().add(&object, 1);
    }

    bool isExistInAttributeGroupSearchTurtle(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        return require_active_group_check_manager().isExist(&object, 0);
    }

    bool isExistInAttributeGroupReflectSpinningBox(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        return require_active_group_check_manager().isExist(&object, 1);
    }
}  // namespace MR
