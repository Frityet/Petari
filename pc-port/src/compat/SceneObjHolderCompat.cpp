#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"

#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/CapturedFrameBlurService.hpp"
#include "compat/GlobalGravityOwnership.hpp"
#include "compat/TalkRuntime.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

    SceneObjHolder *sCurrentSceneObjHolder = nullptr;
    smgpc::scene::SceneObjHolderBinding *sCurrentSceneObjHolderBinding = nullptr;

    [[nodiscard]] bool is_unclaimed_scene_obj_registration(
        const NameObj *object, const void *) noexcept {
        return !smgpc::compat::
                   name_obj_runtime_ownership_is_claimed(object) &&
               !smgpc::scene::
                   current_scene_obj_holder_binding_owns(object);
    }

    void rollback_scene_obj_registrations(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept {
        while (auto *object =
                   smgpc::compat::newest_name_obj_runtime_object_since_if(
                       marker, is_unclaimed_scene_obj_registration,
                       nullptr)) {
            delete object;
        }
    }

}  // namespace

namespace smgpc::scene {

    SceneObjHolderBinding::SceneObjHolderBinding(
        SceneObjHolder &holder,
        SceneObjFactoryOverride factory_override,
        void *factory_context)
        : _holder(&holder), _owned_objects(),
          _owned_registration_objects(),
          _provisional_slots(), _factory_override(factory_override),
          _factory_context(factory_context) {
        smgpc::compat::JkrHostAllocationScope host;
        _global_gravity_ownership = std::make_unique<smgpc::compat::GlobalGravityOwnership>(holder);
        _area_obj_runtime = std::make_unique<AreaObjRuntime>();
        _captured_frame_blur_service = std::make_unique<smgpc::compat::CapturedFrameBlurService>();
        if (sCurrentSceneObjHolder != nullptr) {
            throw std::logic_error("a SceneObjHolder is already bound to the active scene");
        }

        sCurrentSceneObjHolder = _holder;
        sCurrentSceneObjHolderBinding = this;
    }

    SceneObjHolderBinding::~SceneObjHolderBinding() {
        if (sCurrentSceneObjHolder == _holder) {
            sCurrentSceneObjHolder = nullptr;
            sCurrentSceneObjHolderBinding = nullptr;
        }
        // SceneObj dependency order is creation order. Retire each exact
        // object in reverse while the holder is already unavailable, which
        // also prevents destructor-time recreation through MR::createSceneObj.
        while (!_owned_objects.empty()) {
            _owned_objects.pop_back();
        }
        _owned_registration_objects.clear();
        // The external holder storage outlives this binding in test and scene
        // hosts. Reconstruct its exact empty value so no slot retains a freed
        // SceneObj and a later generation can bind/recreate normally.
        *_holder = SceneObjHolder{};
        // PlanetGravityManager and BaseMatrixFollowTargetHolder retain raw
        // pointers into the retail scene heap. Only reclaim their registered
        // children after both SceneObjs have retired.
        _global_gravity_ownership->reclaim();
        _global_gravity_ownership.reset();
        _area_obj_runtime.reset();
        _captured_frame_blur_service.reset();
    }

    void SceneObjHolderBinding::init_after_placement() {
        // Advance by index so callback-time SceneObj creation can append to
        // the graph without invalidating this traversal. A failed callback
        // stays current for an explicit retry; completed and delegated
        // identities are consumed exactly once.
        while (_next_registration_postpass_index <
               _owned_registration_objects.size()) {
            auto *object = _owned_registration_objects[
                _next_registration_postpass_index];
            if (!smgpc::compat::
                     name_obj_runtime_postpass_is_delegated(object)) {
                object->initAfterPlacement();
            }
            ++_next_registration_postpass_index;
        }
        _area_obj_runtime->init_after_placement();
    }

    SceneObjHolder *current_scene_obj_holder() noexcept {
        return sCurrentSceneObjHolder;
    }

    bool current_scene_obj_holder_binding_owns(
        const NameObj *object) noexcept {
        return object != nullptr && sCurrentSceneObjHolderBinding != nullptr &&
               std::ranges::any_of(
                   sCurrentSceneObjHolderBinding
                       ->_owned_registration_objects,
                   [object](const auto *owned) {
                       return owned == object;
                   });
    }

    void adopt_current_scene_obj_holder_descendant(NameObj *object) {
        smgpc::compat::JkrHostAllocationScope host;
        if (object == nullptr || sCurrentSceneObjHolderBinding == nullptr) {
            throw std::logic_error(
                "cannot adopt a NameObj without an active SceneObjHolder binding");
        }
        if (current_scene_obj_holder_binding_owns(object)) {
            return;
        }
        if (!smgpc::compat::has_name_obj_runtime_state(object) ||
            smgpc::compat::name_obj_runtime_ownership_is_claimed(object)) {
            throw std::logic_error(
                "SceneObjHolder descendant must be registered and unclaimed");
        }

        auto *binding = sCurrentSceneObjHolderBinding;
        if (binding->_construction_depth != 0U) {
            // The outermost SceneObj transaction will adopt this identity in
            // exact global registration order after the root finishes init.
            return;
        }
        binding->_owned_objects.reserve(binding->_owned_objects.size() + 1U);
        binding->_owned_registration_objects.reserve(
            binding->_owned_registration_objects.size() + 1U);
        binding->_owned_objects.emplace_back(object);
        binding->_owned_registration_objects.push_back(object);
    }

    AreaObjRuntime *current_area_obj_runtime() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr ? sCurrentSceneObjHolderBinding->_area_obj_runtime.get() : nullptr;
    }

    smgpc::compat::CapturedFrameBlurService *current_captured_frame_blur_service() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr
                   ? sCurrentSceneObjHolderBinding->_captured_frame_blur_service.get()
                   : nullptr;
    }

    smgpc::compat::GlobalGravityOwnership *
    current_global_gravity_ownership() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr ?
                   sCurrentSceneObjHolderBinding->_global_gravity_ownership.get() :
                   nullptr;
    }

}  // namespace smgpc::scene

SceneObjHolder::SceneObjHolder() {
    for (auto &object : mObj) {
        object = nullptr;
    }
}

NameObj *SceneObjHolder::create(int id) {
    if (this != smgpc::scene::current_scene_obj_holder() || sCurrentSceneObjHolderBinding == nullptr ||
        id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }

    if (mObj[id] != nullptr) {
        return mObj[id];
    }

    auto *binding = sCurrentSceneObjHolderBinding;
    const auto marker = smgpc::compat::mark_name_obj_runtime_registrations();
    const auto slot_checkpoint = binding->_provisional_slots.size();
    const auto outermost = binding->_construction_depth == 0U;
    ++binding->_construction_depth;
    auto object = std::unique_ptr<NameObj>{};
    try {
        object.reset(newEachObj(id));
        if (object == nullptr) {
            if (binding->_provisional_slots.size() != slot_checkpoint ||
                smgpc::compat::newest_name_obj_runtime_object_since_if(
                    marker, nullptr, nullptr) != nullptr) {
                throw std::logic_error(
                    "SceneObj factory returned null after creating nested scene objects");
            }
            --binding->_construction_depth;
            return nullptr;
        }

        object->initWithoutIter();
        smgpc::compat::JkrHostAllocationScope host_metadata;
        auto registrations =
            smgpc::compat::snapshot_name_obj_runtime_objects_since(
                marker);
        if (std::ranges::count(registrations, object.get()) != 1 ||
            registrations.empty() || registrations.front() != object.get() ||
            smgpc::compat::name_obj_runtime_ownership_is_claimed(
                object.get()) ||
            smgpc::scene::current_scene_obj_holder_binding_owns(
                object.get())) {
            throw std::logic_error(
                "SceneObj construction did not register one leading, unclaimed root");
        }
        auto *result = object.get();
        binding->_provisional_slots.push_back({id, result});
        mObj[id] = result;
        (void)object.release();

        if (outermost) {
            registrations =
                smgpc::compat::snapshot_name_obj_runtime_objects_since(
                    marker);
            const auto unclaimed_count = static_cast<std::size_t>(
                std::ranges::count_if(
                    registrations, [](const NameObj *registered) {
                        return !smgpc::compat::
                                   name_obj_runtime_ownership_is_claimed(
                                       registered) &&
                               !smgpc::scene::
                                   current_scene_obj_holder_binding_owns(
                                       registered);
                    }));
            binding->_owned_objects.reserve(
                binding->_owned_objects.size() + unclaimed_count);
            binding->_owned_registration_objects.reserve(
                binding->_owned_registration_objects.size() +
                registrations.size());

            for (auto *registered : registrations) {
                if (!smgpc::compat::
                         name_obj_runtime_ownership_is_claimed(registered) &&
                    !smgpc::scene::
                         current_scene_obj_holder_binding_owns(registered)) {
                    binding->_owned_objects.emplace_back(registered);
                }
                if (std::ranges::find(
                        binding->_owned_registration_objects,
                        registered) ==
                    binding->_owned_registration_objects.end()) {
                    binding->_owned_registration_objects.push_back(
                        registered);
                }
            }
            binding->_provisional_slots.clear();
        }
        --binding->_construction_depth;
        return result;
    } catch (...) {
        if (object != nullptr &&
            smgpc::compat::name_obj_runtime_object_was_registered_since(
                object.get(), marker)) {
            (void)object.release();
        }
        while (binding->_provisional_slots.size() > slot_checkpoint) {
            const auto slot = binding->_provisional_slots.back();
            binding->_provisional_slots.pop_back();
            if (mObj[slot.id] == slot.object) {
                mObj[slot.id] = nullptr;
            }
        }
        rollback_scene_obj_registrations(marker);
        --binding->_construction_depth;
        if (outermost) {
            binding->_provisional_slots.clear();
        }
        throw;
    }
}

NameObj *SceneObjHolder::getObj(int id) const {
    if (id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }
    return mObj[id];
}

bool SceneObjHolder::isExist(int id) const {
    return id >= 0 && id < SceneObj_NumMax && mObj[id] != nullptr;
}

NameObj *SceneObjHolder::newEachObj(int id) {
    if (sCurrentSceneObjHolderBinding != nullptr &&
        sCurrentSceneObjHolderBinding->_factory_override != nullptr) {
        if (auto *object =
                sCurrentSceneObjHolderBinding->_factory_override(
                    id,
                    sCurrentSceneObjHolderBinding->_factory_context)) {
            return object;
        }
    }

    switch (id) {
    case SceneObj_ClippingDirector:
        return new ClippingDirector();
    case SceneObj_LightDirector:
        return new LightDirector();
    case SceneObj_PlanetGravityManager:
        return new PlanetGravityManager("重力");
    case SceneObj_BaseMatrixFollowTargetHolder:
        return new BaseMatrixFollowTargetHolder("行列追随先リスト", 256, 256);
    case SceneObj_MessageSensorHolder:
        return new MessageSensorHolder("システム汎用センサー");
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        return new AreaObjContainer("エリアオブジェクトコンテナ管理");
    case SceneObj_PlacementStateChecker:
        return new PlacementStateChecker("オブジェクト配置状態の監視");
    case SceneObj_MarioHolder:
        return new MarioHolder();
    case SceneObj_CoinHolder:
        return new CoinHolder("コイン管理");
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater("コイン回転管理");
    case SceneObj_PrologueHolder:
        return new PrologueHolder("プロローグ保持");
    case SceneObj_CenterScreenBlur:
        return new CenterScreenBlur();
    case SceneObj_InformationObserver:
        return new InformationObserver();
    case SceneObj_TalkDirector:
        return new smgpc::compat::TalkRuntime();
    case SceneObj_LensFlareDirector:
        return new LensFlareDirector();
    case SceneObj_SphereSelector:
        return new SphereSelector();
    case SceneObj_GroupCheckManager:
        return new GroupCheckManager("属性グループマネージャー");
    case SceneObj_PriorDrawAirHolder:
        return new PriorDrawAirHolder();
    default:
        return nullptr;
    }
}

namespace MR {

    NameObj *createSceneObj(int id) {
        auto *holder = getSceneObjHolder();
        return holder != nullptr ? holder->create(id) : nullptr;
    }

    SceneObjHolder *getSceneObjHolder() {
        return smgpc::scene::current_scene_obj_holder();
    }

    bool isExistSceneObj(int id) {
        auto *holder = getSceneObjHolder();
        return holder != nullptr && holder->isExist(id);
    }

}  // namespace MR
