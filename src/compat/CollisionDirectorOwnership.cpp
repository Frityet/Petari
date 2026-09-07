#include "compat/CollisionDirectorOwnership.hpp"

#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionCode.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
#include "scene/StageCollisionService.hpp"
#include <aurora/allocation.hpp>

namespace smgpc::compat {
    CollisionDirectorOwnership::CollisionDirectorOwnership() {
        const aurora::allocation::HostAllocationScope host;
        for (auto& service : _category_services) service = std::make_unique<scene::StageCollisionService>();
    }
    scene::StageCollisionService& CollisionDirectorOwnership::category_service(int category) {
        if (category < 1 || category > 3) aurora::throw_host_exception<std::invalid_argument>("Auxiliary collision category must be 1, 2 or 3.");
        return *_category_services[static_cast<std::size_t>(category - 1)];
    }
    CollisionDirectorOwnership::~CollisionDirectorOwnership() { reclaim(); }

    CollisionDirector* CollisionDirectorOwnership::construct() {
        if (_director != nullptr) {
            aurora::throw_host_exception<std::logic_error>("The scene already owns its CollisionDirector.");
        }
        _director = new CollisionDirector();
        return _director;
    }

    bool CollisionDirectorOwnership::prepare_rollback(NameObjRuntimeRegistrationMarker marker) noexcept {
        if (_director == nullptr || !name_obj_runtime_object_was_registered_since(_director, marker)) return false;
        prepare_retirement();
        return true;
    }

    void CollisionDirectorOwnership::prepare_retirement() noexcept {
        if (_prepared || _director == nullptr) return;
        _keepers = _director->mCategoryKeeper;
        _code = _director->mCode;
        for (std::size_t category = 0; category < _hit_infos.size(); ++category) {
            auto* keeper = _keepers[category];
            _hit_infos[category] = keeper->mHitInfoArray;
            for (int zone = 0; zone < keeper->mZoneNum; ++zone) {
                _zones[category][zone] = keeper->mZones[zone];
            }
        }
        _prepared = true;
    }

    void CollisionDirectorOwnership::reclaim() noexcept {
        if (!_prepared) return;
        for (auto& zones : _zones) {
            for (auto*& zone : zones) { delete zone; zone = nullptr; }
        }
        for (auto*& infos : _hit_infos) { delete[] infos; infos = nullptr; }
        if (_code != nullptr) {
            for (auto* table : {_code->mFloorTable, _code->mWallTable, _code->mSoundTable, _code->mCameraTable}) {
                delete[] table->mHashTable;
                delete[] table->mCodeTable;
                delete[] table->mNameTable;
                delete table;
            }
            delete _code;
            _code = nullptr;
        }
        delete[] _keepers;
        _keepers = nullptr;
        _director = nullptr;
        _prepared = false;
    }
}
