#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include <aurora/exception.hpp>
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "compat/EffectSystemOwnership.hpp"
#include "compat/ImageEffectOwnership.hpp"
#include "compat/CollisionDirectorOwnership.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/SunshadeMapHolder.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "runtime/SceneScheduler.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"

#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "compat/DemoDirectorOwnership.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/SensorHitChecker.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/PlayerEvent.hpp"
#include "Game/GameAudio/AudBgmConductor.hpp"
#include "Game/MapObj/BigFanHolder.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Screen/CinemaFrame.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Map/NamePosHolder.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/NameObj/MovementOnOffGroupHolder.hpp"
#include "Game/NPC/NPCDirector.hpp"
#include "Game/Util/FurCtrl.hpp"
#include "Game/Util/ShareUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/CapturedFrameBlurService.hpp"
#include "compat/GlobalGravityOwnership.hpp"
#include "compat/TalkRuntime.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cCameraDirectorName = encode_owner_name("カメラ管理");
    const std::string cGravityManagerName = encode_owner_name("重力");
    const std::string cBaseMatrixFollowTargetHolderName = encode_owner_name("行列追随先リスト");
    const std::string cLiveActorGroupArrayName = encode_owner_name("オブジェクトグループ");
    const std::string cMovementOnOffGroupHolderName = encode_owner_name("Movementグループ管理");
    const std::string cMessageSensorHolderName = encode_owner_name("システム汎用センサー");
    const std::string cAreaObjContainerName = encode_owner_name("エリアオブジェクトコンテナ管理");
    const std::string cPlacementStateCheckerName = encode_owner_name("オブジェクト配置状態の監視");
    const std::string cWarpPodManagerName = encode_owner_name("ワープポッド管理局");
    const std::string cCoinHolderName = encode_owner_name("コイン管理");
    const std::string cClipAreaHolderName = encode_owner_name("クリップエリアホルダー");
    const std::string cCoinRotaterName = encode_owner_name("コイン回転管理");
    const std::string cPrologueHolderName = encode_owner_name("プロローグ保持");
    const std::string cGroupCheckManagerName = encode_owner_name("属性グループマネージャー");
}  // namespace

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

    std::shared_ptr<smgpc::compat::JkrAllocationDomain> current_scene_allocation_domain() noexcept {
        return sCurrentSceneObjHolderBinding ? sCurrentSceneObjHolderBinding->_game_allocation_domain : nullptr;
    }

    smgpc::compat::CollisionDirectorOwnership* current_collision_director_ownership() noexcept {
        return sCurrentSceneObjHolderBinding ? sCurrentSceneObjHolderBinding->_collision_director_ownership.get() : nullptr;
    }

    SceneObjHolderBinding::SceneObjHolderBinding(
        SceneObjHolder &holder,
        SceneObjFactoryOverride factory_override,
        void *factory_context,
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> allocation_domain)
        : _game_allocation_domain(std::move(allocation_domain)), _holder(&holder), _owned_objects(),
          _owned_registration_objects(),
          _provisional_slots(), _factory_override(factory_override),
          _factory_context(factory_context) {
        smgpc::compat::JkrHostAllocationScope host;
        _global_gravity_ownership = std::make_unique<smgpc::compat::GlobalGravityOwnership>(holder);
        _collision_director_ownership = std::make_unique<smgpc::compat::CollisionDirectorOwnership>();
        _demo_director_ownership = std::make_unique<smgpc::compat::DemoDirectorOwnership>();
        _image_effect_ownership = std::make_unique<smgpc::compat::ImageEffectOwnership>(holder);
        _area_obj_runtime = std::make_unique<AreaObjRuntime>();
        _captured_frame_blur_service = std::make_unique<smgpc::compat::CapturedFrameBlurService>();
        if (sCurrentSceneObjHolder != nullptr) {
            aurora::throw_host_exception<std::logic_error>("a SceneObjHolder is already bound to the active scene");
        }

        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) {
                if (!_game_allocation_domain) {
                    _game_allocation_domain = smgpc::compat::JkrAllocationDomain::create(runtime->host_heaps(), 8U * 1024U * 1024U);
                }
                _game_allocation_binding = std::make_unique<smgpc::runtime::SceneSchedulerAllocationBinding>(*scheduler, _game_allocation_domain);
            }
        }

        sCurrentSceneObjHolder = _holder;
        sCurrentSceneObjHolderBinding = this;
        try {
            if (auto* messages = smgpc::runtime::current_message_holder())
                _scene_messages = std::make_unique<smgpc::runtime::SceneMessageBinding>(*messages);
            // Every original LiveActor joins this group during construction,
            // including actors with no movement or draw registration.
            if (dynamic_cast<AllLiveActorGroup*>(_holder->create(SceneObj_AllLiveActorGroup)) == nullptr) {
                aurora::throw_host_exception<std::logic_error>("Scene initialization requires the original AllLiveActorGroup");
            }
        } catch (...) {
            sCurrentSceneObjHolder = nullptr;
            sCurrentSceneObjHolderBinding = nullptr;
            *_holder = SceneObjHolder{};
            throw;
        }
    }

    SceneObjHolderBinding::~SceneObjHolderBinding() {
        _demo_director_ownership->prepare_retirement();
        _collision_director_ownership->prepare_retirement();
        _image_effect_ownership->prepare_retirement();
        if (_camera_runtime) _camera_runtime->unpublish();
        if (_effect_scheduler) {
            (void)_effect_scheduler->remove_registrations_since(_effect_registration_marker);
        }
        if (_effect_system_ownership) _effect_system_ownership->retire();
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
        _demo_director_ownership->reclaim();
        _demo_director_ownership.reset();
        smgpc::compat::release_scene_collision_parts(_holder);
        _collision_director_ownership->reclaim();
        _collision_director_ownership.reset();
        _image_effect_ownership->reclaim_prepared();
        _image_effect_ownership.reset();
        _owned_registration_objects.clear();
        _camera_runtime.reset();
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
        _effect_system_ownership.reset();
        _scene_messages.reset();
        _game_allocation_binding.reset();
        _game_allocation_domain.reset();
    }

    void SceneObjHolderBinding::initialize_effect_system(unsigned particles, unsigned emitters, std::size_t byte_budget) {
        smgpc::compat::JkrHostAllocationScope host;
        if (_effect_system_ownership || _holder->isExist(SceneObj_EffectSystem))
            aurora::throw_host_exception<std::logic_error>("scene effect system already initialized");
        _effect_scheduler = smgpc::runtime::try_active_scene_scheduler();
        if (!_effect_scheduler) aurora::throw_host_exception<std::logic_error>("EffectSystem requires the active scene scheduler");
        _effect_registration_marker = _effect_scheduler->registration_marker();
        _effect_system_ownership = std::make_unique<smgpc::compat::EffectSystemOwnership>(byte_budget);
        _holder->create(SceneObj_EffectSystem);
        _effect_system_ownership->entry(particles, emitters);
    }

    smgpc::compat::EffectSystemOwnership* current_effect_system_ownership() noexcept {
        return sCurrentSceneObjHolderBinding ? sCurrentSceneObjHolderBinding->_effect_system_ownership.get() : nullptr;
    }

    void SceneObjHolderBinding::initialize_camera_system() {
        smgpc::compat::JkrHostAllocationScope host;
        if (_camera_runtime) {
            aurora::throw_host_exception<std::logic_error>("scene camera system already initialized");
        }
        _holder->create(SceneObj_CameraContext);
        _holder->create(SceneObj_CameraDirector);
        if (!_camera_runtime)
            aurora::throw_host_exception<std::logic_error>("Camera initialization requires the actual camera SceneObjs");
    }

    void SceneObjHolderBinding::init_after_placement() {
        const auto phase = SceneInitializationScope(SceneInitializeState_AfterPlacement);
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
                if (_game_allocation_domain) {
                    smgpc::compat::JkrAllocationScope heap(_game_allocation_domain);
                    object->initAfterPlacement();
                } else {
                    object->initAfterPlacement();
                }
            }
            ++_next_registration_postpass_index;
        }
        _area_obj_runtime->init_after_placement();
    }

    void SceneObjHolderBinding::acknowledge_scene_postpass(std::span<NameObj *const> objects) {
        // Original NameObjHolder captures its end pointer before dispatch.
        // Objects appended by a callback do not belong to that completed pass.
        while (_next_registration_postpass_index < _owned_registration_objects.size() &&
               std::ranges::find(objects, _owned_registration_objects[_next_registration_postpass_index]) != objects.end())
            ++_next_registration_postpass_index;
        _area_obj_runtime->acknowledge_scene_postpass(objects);
    }

    void SceneObjHolderBinding::complete_camera_parameters() {
        if (_camera_runtime) {
            smgpc::compat::JkrAllocationScope heap(_game_allocation_domain);
            _camera_runtime->close_creating_chunks();
        }
    }

    void SceneObjHolderBinding::complete_initialization() {
        _initialization_state.complete();
    }

    smgpc::compat::DemoDirectorOwnership* current_demo_director_ownership() noexcept {
        return sCurrentSceneObjHolderBinding ? sCurrentSceneObjHolderBinding->_demo_director_ownership.get() : nullptr;
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
            aurora::throw_host_exception<std::logic_error>(
                "cannot adopt a NameObj without an active SceneObjHolder binding");
        }
        if (current_scene_obj_holder_binding_owns(object)) {
            return;
        }
        if (!smgpc::compat::has_name_obj_runtime_state(object) ||
            smgpc::compat::name_obj_runtime_ownership_is_claimed(object)) {
            aurora::throw_host_exception<std::logic_error>(
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
    smgpc::compat::JutTextureConstructionScope textures(
        smgpc::compat::ImageEffectOwnership::handles(id));
    try {
        std::optional<smgpc::compat::JkrAllocationScope> game_heap;
        if (binding->_game_allocation_domain) {
            game_heap.emplace(binding->_game_allocation_domain);
        }
        object.reset(newEachObj(id));
        if (object == nullptr) {
            if (binding->_provisional_slots.size() != slot_checkpoint ||
                smgpc::compat::newest_name_obj_runtime_object_since_if(
                    marker, nullptr, nullptr) != nullptr) {
                aurora::throw_host_exception<std::logic_error>(
                    "SceneObj factory returned null after creating nested scene objects");
            }
            --binding->_construction_depth;
            return nullptr;
        }

        binding->_image_effect_ownership->capture(id, *object, textures);
        if (id == SceneObj_DemoDirector) binding->_demo_director_ownership->capture(static_cast<DemoDirector&>(*object));
        object->initWithoutIter();
        binding->_image_effect_ownership->capture(id, *object, textures);
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
            aurora::throw_host_exception<std::logic_error>(
                "SceneObj construction did not register one leading, unclaimed root");
        }
        auto *result = object.get();
        binding->_provisional_slots.push_back({id, result});
        mObj[id] = result;
        (void)object.release();

        if (id == SceneObj_CameraDirector) {
            auto *context = dynamic_cast<CameraContext *>(mObj[SceneObj_CameraContext]);
            auto *director = dynamic_cast<CameraDirector *>(result);
            if (!context || !director)
                aurora::throw_host_exception<std::logic_error>("Camera publication requires the actual CameraContext and CameraDirector");
            binding->_camera_runtime = std::make_unique<smgpc::camera::CameraDirectorRuntime>(*context, *director);
        }

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
        if (binding->_camera_runtime && smgpc::compat::name_obj_runtime_object_was_registered_since(
                &binding->_camera_runtime->director(), marker))
            binding->_camera_runtime.reset();
        binding->_image_effect_ownership->capture_shared_textures(textures);
        binding->_demo_director_ownership->prepare_rollback(marker);
        binding->_image_effect_ownership->prepare_rollback(marker);
        const bool collision_rollback = binding->_collision_director_ownership->prepare_rollback(marker);
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
        binding->_demo_director_ownership->reclaim();
        binding->_image_effect_ownership->reclaim_prepared();
        if (collision_rollback) binding->_collision_director_ownership->reclaim();
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

    if (smgpc::compat::ImageEffectOwnership::handles(id)) {
        return sCurrentSceneObjHolderBinding->_image_effect_ownership->construct(id);
    }

    switch (id) {
    case SceneObj_AllLiveActorGroup:
        return new AllLiveActorGroup();
    case SceneObj_SunshadeMapHolder:
        return new SunshadeMapHolder();
    case SceneObj_CollisionDirector:
        return sCurrentSceneObjHolderBinding->_collision_director_ownership->construct();
    case SceneObj_CameraContext:
        return new CameraContext();
    case SceneObj_CameraDirector:
        return new CameraDirector(cCameraDirectorName.c_str());
    case SceneObj_EffectSystem:
        if (!sCurrentSceneObjHolderBinding->_effect_system_ownership)
            aurora::throw_host_exception<std::logic_error>("EffectSystem requires scene heap initialization");
        return sCurrentSceneObjHolderBinding->_effect_system_ownership->construct();
    case SceneObj_ClippingDirector:
        return new ClippingDirector();
    case SceneObj_LightDirector:
        return new LightDirector();
    case SceneObj_FurDrawManager:
        return new FurDrawManager(64);
    case SceneObj_PlanetGravityManager:
        return new PlanetGravityManager(cGravityManagerName.c_str());
    case SceneObj_MovementOnOffGroupHolder:
        return new MovementOnOffGroupHolder(cMovementOnOffGroupHolderName.c_str());
    case SceneObj_NPCDirector:
        return new NPCDirector();
    case SceneObj_LiveActorGroupArray:
        return new LiveActorGroupArray(cLiveActorGroupArrayName.c_str());
    case SceneObj_BaseMatrixFollowTargetHolder:
        return new BaseMatrixFollowTargetHolder(cBaseMatrixFollowTargetHolderName.c_str(), 256, 256);
    case SceneObj_MessageSensorHolder:
        return new MessageSensorHolder(cMessageSensorHolderName.c_str());
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        return new AreaObjContainer(cAreaObjContainerName.c_str());
    case SceneObj_PlacementStateChecker:
        return new PlacementStateChecker(cPlacementStateCheckerName.c_str());
    case SceneObj_MarioHolder:
        return new MarioHolder();
    case SceneObj_BigFanHolder:
        return new BigFanHolder();
    case SceneObj_WarpPodMgr:
        return new WarpPodMgr(cWarpPodManagerName.c_str());
    case SceneObj_CoinHolder:
        return new CoinHolder(cCoinHolderName.c_str());
    case SceneObj_ResourceShare:
        return new ResourceShare();
    case SceneObj_ClipAreaHolder:
        return new ClipAreaHolder(cClipAreaHolderName.c_str());
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater(cCoinRotaterName.c_str());
    case SceneObj_SensorHitChecker:
        return new SensorHitChecker("SensorHitChecker");
    case SceneObj_DemoDirector:
        return new DemoDirector("DemoDirector");
    case SceneObj_PrologueHolder:
        return new PrologueHolder(cPrologueHolderName.c_str());
    case SceneObj_CaptureScreenActor:
        return new CaptureScreenActor(MR::DrawType_CaptureScreenIndirect, "Indirect");
    case SceneObj_CenterScreenBlur:
        return new CenterScreenBlur();
    case SceneObj_InformationObserver:
        return new InformationObserver();
    case SceneObj_NameObjExecuteHolder:
        return new NameObjExecuteHolder(4096);
    case SceneObj_StopSceneController:
        return new StopSceneController();
    case SceneObj_SceneNameObjMovementController:
        return new SceneNameObjMovementController();
    case SceneObj_NameObjGroup:
        return new NameObjGroup("IgnorePauseNameObj", 16);
    case SceneObj_NamePosHolder:
        return new NamePosHolder();
    case SceneObj_CinemaFrame:
        return new CinemaFrame(true);
    case SceneObj_SceneWipeHolder:
        return new SceneWipeHolder();
    case SceneObj_GameSceneLayoutHolder:
        return new GameSceneLayoutHolder();
    case SceneObj_ShadowControllerHolder:
        return new ShadowControllerHolder();
    case SceneObj_AudBgmConductor:
        return new AudBgmConductor();
    case SceneObj_EventSequencer:
        return new EventSequencer();
    case SceneObj_ScenePlayingResult:
        return new ScenePlayingResult();
    case SceneObj_TalkDirector:
        return new smgpc::compat::TalkRuntime();
    case SceneObj_LensFlareDirector:
        return new LensFlareDirector();
    case SceneObj_SphereSelector:
        return new SphereSelector();
    case SceneObj_GroupCheckManager:
        return new GroupCheckManager(cGroupCheckManagerName.c_str());
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
