#include "compat/ClippingDirectorOwnership.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/LiveActor/ClippingActorInfo.hpp"
#include "Game/LiveActor/ClippingGroupHolder.hpp"
#include "Game/LiveActor/ViewGroupCtrl.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace smgpc::compat {
    ClippingDirectorOwnership::~ClippingDirectorOwnership() { reclaim(); }

    ClippingDirector* ClippingDirectorOwnership::construct() {
        if (_director != nullptr)
            aurora::throw_host_exception<std::logic_error>("The scene already owns its ClippingDirector.");
        _director = new ClippingDirector();
        _actors = _director->mActorHolder;
        _groups = _director->mGroupHolder;
        _lists = {_actors->_10, _actors->_14, _actors->_18, _actors->_1C};
        _view = _actors->mViewGroupCtrl;
        _group_array = _groups->mInfoGroups;
        return _director;
    }

    void ClippingDirectorOwnership::capture_groups() noexcept {
        if (_prepared || _groups == nullptr) return;
        for (s32 i = 0; i < _groups->mNumGroups; ++i) {
            auto* group = _groups->mInfoGroups[i];
            GroupStorage* empty = nullptr;
            bool captured = false;
            for (auto& record : _group_storage) {
                if (record.object == group) { captured = true; break; }
                if (record.object == nullptr && empty == nullptr) empty = &record;
            }
            if (!captured && empty != nullptr) *empty = {group, group->_14, group->_18};
        }
    }

    void ClippingDirectorOwnership::release_name_obj(const NameObj* object) noexcept {
        if (_prepared) return;
        if (object == _director || object == _actors || object == _groups) {
            prepare_retirement();
            return;
        }
        for (auto& record : _group_storage) {
            if (record.object != object) continue;
            // Only borrowed identity is read here: the original derived
            // destructor has already returned when NameObj invokes this hook.
            if (_groups != nullptr) {
                for (s32 i = 0; i < _groups->mNumGroups; ++i) {
                    if (_groups->mInfoGroups[i] == record.object) {
                        _groups->mInfoGroups[i] = _groups->mInfoGroups[--_groups->mNumGroups];
                        break;
                    }
                }
            }
            delete[] record.infos;
            delete record.id;
            record = {};
            return;
        }
    }

    bool ClippingDirectorOwnership::prepare_rollback(NameObjRuntimeRegistrationMarker marker) noexcept {
        if (_director == nullptr || !name_obj_runtime_object_was_registered_since(_director, marker)) return false;
        prepare_retirement();
        return true;
    }

    void ClippingDirectorOwnership::prepare_retirement() noexcept {
        if (_prepared || _director == nullptr) return;
        // Stable raw storage was captured while the original owners were
        // alive. Group storage is captured after each original group join.
        // This also permits the NameObj base-destructor notification path
        // without accessing any retired derived object's fields.
        retire_clipping_actor_holder(*_actors);
        retire_clipping_group_holder(*_groups);
        _prepared = true;
    }

    void ClippingDirectorOwnership::reclaim() noexcept {
        if (!_prepared) return;
        for (auto*& list : _lists) {
            for (s32 i = 0; i < list->_4; ++i) {
                delete list->mClippingActorList[i]->mInfo;
                delete list->mClippingActorList[i];
            }
            delete[] list->mClippingActorList;
            delete list;
            list = nullptr;
        }
        delete[] _view->mViewGroupData;
        delete[] _view->mLodCtrls;
        delete _view;
        for (auto& record : _group_storage) {
            delete[] record.infos;
            delete record.id;
            record = {};
        }
        delete[] _group_array;
        _view = nullptr;
        _group_array = nullptr;
        _director = nullptr;
        _actors = nullptr;
        _groups = nullptr;
        _prepared = false;
    }
}
