#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"

#include <stdexcept>

namespace {
    smgpc::runtime::SceneScheduler &require_scheduler() {
        if (auto *scheduler = smgpc::runtime::try_active_scene_scheduler(); scheduler != nullptr) {
            return *scheduler;
        }
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->scheduler();
        }
        throw std::logic_error("Scene movement category requests require an active scene scheduler.");
    }
}  // namespace

namespace MR {
    void requestMovementOnWithCategory(int category) {
        require_scheduler().request_movement_on(category);
    }

    void requestMovementOffWithCategory(int category) {
        require_scheduler().request_movement_off(category);
    }
}  // namespace MR

void CategoryList::requestMovementOn(MR::MovementType type) {
    MR::requestMovementOnWithCategory(type);
}

void CategoryList::requestMovementOff(MR::MovementType type) {
    MR::requestMovementOffWithCategory(type);
}
