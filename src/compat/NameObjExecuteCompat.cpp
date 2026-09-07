#include <aurora/exception.hpp>
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <cstring>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/LightUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>

NameObj* NameObjFinder::find(const char* name) {
    auto* registry = smgpc::scene::current_scene_name_obj_registry();
    if (!registry)
        aurora::throw_host_exception<std::logic_error>("Name lookup requires the actual scene NameObjHolder");
    return registry->holder().find(name);
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
