#include "compat/ShadowControllerOwnership.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/exception.hpp>

#include <algorithm>
#include <stdexcept>
#include <limits>
#include <string>

namespace smgpc::compat {
    struct ShadowControllerOwnership::Entry {
        ShadowControllerOwnership* owner;
        std::string name;
        std::string group_name;
        ActorShadowPositionBinding position_binding;
        std::unique_ptr<ShadowController> controller;

        ~Entry() {
            if (controller) owner->remove_from_holder(controller.get());
        }
    };

    ShadowControllerOwnership::ShadowControllerOwnership(
        LiveActor& actor, const ActorShadowRuntimeState& definitions)
        : _domain(scene::current_scene_allocation_domain()), _actor(&actor), _holder(nullptr), _holder_generation(0) {
        JkrHostAllocationScope host;
        if (!_domain || !scene::current_scene_obj_holder()) {
            aurora::throw_host_exception<std::logic_error>("Original shadow controllers require the active scene Game heap");
        }
        if (definitions.capacity > static_cast<std::uint32_t>(std::numeric_limits<s32>::max()) ||
            definitions.controllers.size() > definitions.capacity) {
            aurora::throw_host_exception<std::length_error>("Shadow definitions exceed the original controller list capacity");
        }
        _entries.reserve(definitions.capacity);
        {
            JkrAllocationScope game(_domain);
            _list = std::make_unique<ShadowControllerList>(&actor, definitions.capacity);
        }
        for (const auto& definition : definitions.controllers) add(definition);
    }

    ShadowControllerOwnership::~ShadowControllerOwnership() {
        if (_actor->mShadowControllerList == _list.get()) _actor->mShadowControllerList = nullptr;
        _entries.clear();
        _list.reset();
    }

    void ShadowControllerOwnership::publish() {
        _actor->mShadowControllerList = _list.get();
    }

    void ShadowControllerOwnership::add(const ActorShadowControllerRuntimeState& definition) {
        JkrHostAllocationScope host;
        if (scene::current_scene_allocation_domain() != _domain) {
            aurora::throw_host_exception<std::logic_error>("Adding a shadow requires its active owning scene");
        }
        if (!_holder) {
            JkrAllocationScope game(_domain);
            _holder = static_cast<ShadowControllerHolder*>(MR::createSceneObj(SceneObj_ShadowControllerHolder));
            if (!_holder) {
                aurora::throw_host_exception<std::logic_error>("Original shadow controller holder is unavailable");
            }
            _holder_generation = name_obj_runtime_generation(_holder);
        }
        if (name_obj_runtime_generation(_holder) != _holder_generation) {
            aurora::throw_host_exception<std::logic_error>("Original shadow controller holder has retired");
        }
        if (_list->mShadowList.size() >= _list->mShadowList.capacity() ||
            _holder->_C.size() >= _holder->_C.capacity()) {
            aurora::throw_host_exception<std::length_error>("Original shadow controller capacity exhausted");
        }
        auto entry = std::make_unique<Entry>();
        entry->owner = this;
        entry->name = definition.name;
        entry->group_name = definition.group_name;
        entry->position_binding = definition.position_binding;
        {
            JkrAllocationScope game(_domain);
            entry->controller = std::make_unique<ShadowController>(_actor, entry->name.c_str());
        }
        auto& controller = *entry->controller;
        controller.setGroupName(entry->group_name.c_str());
        if (definition.drop_position) {
            controller.setDropPosPtr(definition.drop_position);
        } else if (definition.drop_position_matrix) {
            controller.setDropPosMtxPtr(definition.drop_position_matrix, definition.drop_offset);
        } else {
            controller.setDropPosFix(definition.fixed_drop_position);
        }
        if (definition.drop_direction) controller.setDropDirPtr(definition.drop_direction);
        else controller.setDropDirFix(definition.fixed_drop_direction);
        controller.setDropLength(definition.drop_length);
        controller.setDropStartOffset(definition.drop_start_offset);
        if (definition.kind == ActorShadowControllerKind::SurfaceCircle ||
            definition.kind == ActorShadowControllerKind::SurfaceOval ||
            definition.kind == ActorShadowControllerKind::SurfaceBox) controller.setDropTypeSurface();
        else controller.setDropTypeNormal();
        switch (definition.calculation_mode) {
        case ActorShadowCalculationMode::Disabled: controller.offCalcCollision(); break;
        case ActorShadowCalculationMode::Continuous: controller.onCalcCollision(); break;
        case ActorShadowCalculationMode::OneTime: controller.onCalcCollisionOneTime(); break;
        }
        switch (definition.gravity_mode) {
        case ActorShadowGravityMode::HostDirection: controller.offCalcDropGravity(); break;
        case ActorShadowGravityMode::HostContinuous: controller.onCalcDropGravity(); break;
        case ActorShadowGravityMode::HostOneTime: controller.onCalcDropGravityOneTime(); break;
        case ActorShadowGravityMode::PrivateDisabled: controller.offCalcDropPrivateGravity(); break;
        case ActorShadowGravityMode::PrivateContinuous: controller.onCalcDropPrivateGravity(); break;
        case ActorShadowGravityMode::PrivateOneTime: controller.onCalcDropPrivateGravityOneTime(); break;
        }
        if (definition.follow_host_scale) controller.onFollowHostScale();
        else controller.offFollowHostScale();
        if (definition.visible_sync_host) controller.onVisibleSyncHost();
        else controller.offVisibleSyncHost();
        if (definition.valid) controller.validate();
        else controller.invalidate();
        _list->addController(&controller);
        _entries.push_back(std::move(entry));
    }

    void ShadowControllerOwnership::remove_from_holder(ShadowController* controller) noexcept {
        if (!_holder || name_obj_runtime_generation(_holder) != _holder_generation) return;
        for (auto* list : {&_holder->_C, &_holder->_18}) {
            auto* begin = list->mArray.mArr;
            auto* end = begin + list->mCount;
            list->mCount = static_cast<s32>(std::remove(begin, end, controller) - begin);
        }
    }

    void ShadowControllerOwnership::invalidate_joint_matrices() noexcept {
        for (const auto& entry : _entries) {
            if (entry->position_binding == ActorShadowPositionBinding::JointMatrix) {
                entry->controller->invalidate();
                entry->controller->_18 = nullptr;
                entry->controller->_1C = nullptr;
            }
        }
    }
}
