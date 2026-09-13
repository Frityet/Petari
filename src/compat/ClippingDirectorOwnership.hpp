#pragma once

#include "compat/ActorRuntimeRegistry.hpp"
#include <array>

class NameObj;
class ClippingDirector;
class ClippingActorHolder;
class ClippingActorInfo;
class ClippingActorInfoList;
class ClippingGroupHolder;
class ClippingInfoGroup;
class JMapIdInfo;
class ViewGroupCtrl;

namespace smgpc::compat {
    // Original NameObjs remain scene-owned; this boundary reclaims their raw
    // storage and tracks individual group retirement without changing Game.
    class ClippingDirectorOwnership final {
    public:
        ~ClippingDirectorOwnership();
        ClippingDirector* construct();
        void capture_groups() noexcept;
        void release_name_obj(const NameObj*) noexcept;
        bool prepare_rollback(NameObjRuntimeRegistrationMarker) noexcept;
        void prepare_retirement() noexcept;
        void reclaim() noexcept;

    private:
        struct GroupStorage {
            ClippingInfoGroup* object = nullptr;
            ClippingActorInfo** infos = nullptr;
            JMapIdInfo* id = nullptr;
        };
        ClippingDirector* _director = nullptr;
        ClippingActorHolder* _actors = nullptr;
        ClippingGroupHolder* _groups = nullptr;
        std::array<ClippingActorInfoList*, 4> _lists{};
        ViewGroupCtrl* _view = nullptr;
        ClippingInfoGroup** _group_array = nullptr;
        std::array<GroupStorage, 64> _group_storage{};
        bool _prepared = false;
    };
}
