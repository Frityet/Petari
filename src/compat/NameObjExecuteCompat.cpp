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
