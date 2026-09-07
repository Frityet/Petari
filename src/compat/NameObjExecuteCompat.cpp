#include <aurora/exception.hpp>
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <cstring>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/LightUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>

namespace {
    smgpc::runtime::SceneScheduler &require_scheduler(NameObj *object) {
        if (object == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Temporary draw connection requires a NameObj.");
        }
        if (auto *scheduler = smgpc::runtime::try_active_scene_scheduler(); scheduler != nullptr) {
            return *scheduler;
        }
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->scheduler();
        }
        aurora::throw_host_exception<std::logic_error>("Temporary draw connection requires an active runtime scene.");
    }
}

namespace MR {
    void connectToDrawTemporarily(NameObj *object) {
        require_scheduler(object).connect_draw(*object);
    }

    void disconnectToDrawTemporarily(NameObj *object) {
        require_scheduler(object).disconnect_draw(*object);
    }

    bool isConnectToDrawTemporarily(const NameObj *object) {
        auto &scheduler = require_scheduler(const_cast<NameObj *>(object));
        return scheduler.is_draw_connected(*object);
    }

    void findActorLightInfo(const LiveActor *actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Actor light lookup requires a LiveActor.");
        }
        auto &scheduler = require_scheduler(const_cast<LiveActor *>(actor));
        scheduler.find_actor_light_info(*const_cast<LiveActor *>(actor));
    }
}

NameObj* NameObjFinder::find(const char* name) {
    const smgpc::compat::JkrHostAllocationScope host;
    // The native registry is the active NameObjHolder. Its snapshot retains
    // construction order, including unplaced controller and layout objects.
    for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects()) {
        if (std::strcmp(object->getName(), name) == 0)
            return object;
    }
    return nullptr;
}

namespace MR {
    NameObjGroup* joinToNameObjGroup(NameObj* object, const char* group_name) {
        auto* group = static_cast<NameObjGroup*>(NameObjFinder::find(group_name));
        if (!group)
            aurora::throw_host_exception<std::logic_error>("Joining a NameObj group requires its scene-owned group to be created first");
        group->registerObj(object);
        return group;
    }
}
