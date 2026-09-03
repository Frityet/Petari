#include "compat/MaterialCtrlCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/J3dMatrix.hpp"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    using ControllerList = std::vector<std::unique_ptr<ProjmapEffectMtxSetter>>;

    auto& actor_controllers() {
        static auto controllers = std::unordered_map<const LiveActor*, ControllerList>{};
        return controllers;
    }

    struct ProjectionState {
        LiveActor* actor;
        smgpc::render::J3dMatrix3x4 matrix;
    };

    auto& controller_actors() {
        static auto actors = std::unordered_map<const ProjmapEffectMtxSetter*, ProjectionState>{};
        return actors;
    }

    LiveActor& require_controller_actor(const ProjmapEffectMtxSetter* controller) {
        const auto found = controller_actors().find(controller);
        if (found == controller_actors().end() || found->second.actor == nullptr) {
            throw std::logic_error("Projection material controller is not bound to a real LiveActor model.");
        }
        if (smgpc::compat::actor_model(found->second.actor) == nullptr) {
            throw std::logic_error("Projection material controller has no real LiveActor model renderer.");
        }
        return *found->second.actor;
    }

    const smgpc::render::J3dMatrix3x4& require_invertible_base_transform(const LiveActor& actor) {
        const auto& matrix = smgpc::compat::actor_base_matrix(&actor);
        for (const auto value : matrix.m) {
            if (!std::isfinite(value)) {
                throw std::logic_error("Projection material controller requires a finite model transform.");
            }
        }

        const auto determinant =
            matrix.m[0U] * ((matrix.m[5U] * matrix.m[10U]) - (matrix.m[6U] * matrix.m[9U])) -
            matrix.m[1U] * ((matrix.m[4U] * matrix.m[10U]) - (matrix.m[6U] * matrix.m[8U])) +
            matrix.m[2U] * ((matrix.m[4U] * matrix.m[9U]) - (matrix.m[5U] * matrix.m[8U]));
        if (!std::isfinite(determinant) || std::abs(determinant) < 0.000001F) {
            throw std::logic_error("Projection material controller requires an invertible model transform.");
        }
        return matrix;
    }

    void apply_projection_matrix(ProjmapEffectMtxSetter* controller,
                                 const smgpc::render::J3dMatrix3x4& matrix) {
        auto& actor = require_controller_actor(controller);
        controller_actors().at(controller).matrix = matrix;
        smgpc::compat::set_actor_projmap_effect_matrix(&actor, matrix);
    }
}

MaterialCtrl::MaterialCtrl(J3DModelData* modelData, const char* materialName)
    : mModelData(modelData), mMaterial(nullptr) {
    if (modelData != nullptr || materialName != nullptr) {
        throw std::logic_error("Raw J3D material controllers are unavailable without a real host J3D material table.");
    }
}

void MaterialCtrl::update() {
    if (const auto* controller = dynamic_cast<const ProjmapEffectMtxSetter*>(this)) {
        auto& actor = require_controller_actor(controller);
        smgpc::compat::set_actor_projmap_effect_matrix(&actor, controller_actors().at(controller).matrix);
        return;
    }
    throw std::logic_error("Raw J3D material controller update is unavailable without a real host J3D material table.");
}

void MaterialCtrl::updateMaterial(J3DMaterial*) {
    throw std::logic_error("Raw J3D material update is unavailable without a real host J3D material table.");
}

ProjmapEffectMtxSetter::ProjmapEffectMtxSetter(J3DModel* model, const ResourceHolder* resourceHolder)
    : MaterialCtrl(nullptr, nullptr), temp{} {
    if (model != nullptr || resourceHolder != nullptr) {
        throw std::logic_error("Raw J3D projection material construction is unavailable without real J3D model resources.");
    }
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtx() {
    const auto& actor = require_controller_actor(this);
    apply_projection_matrix(this, smgpc::render::j3d_invert_affine_matrix(require_invertible_base_transform(actor)));
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtxWithLocalOffset(const TVec3f& offset) {
    const auto& actor = require_controller_actor(this);
    const auto localOffset = smgpc::render::J3dMatrix3x4{{
        1.0F, 0.0F, 0.0F, offset.x,
        0.0F, 1.0F, 0.0F, offset.y,
        0.0F, 0.0F, 1.0F, offset.z,
    }};
    apply_projection_matrix(
        this, smgpc::render::j3d_invert_affine_matrix(
                  smgpc::render::j3d_concat_matrix(require_invertible_base_transform(actor), localOffset)));
}

namespace smgpc::compat {
    const smgpc::render::J3dMatrix3x4& projmap_effect_matrix(const ProjmapEffectMtxSetter* controller) {
        require_controller_actor(controller);
        return controller_actors().at(controller).matrix;
    }

    ProjmapEffectMtxSetter* create_projmap_effect_mtx_setter(LiveActor* actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("Projection material controller requires a LiveActor.");
        }
        if (actor_model(actor) == nullptr) {
            throw std::logic_error("Projection material controller requires a real LiveActor model renderer.");
        }

        auto controller = std::make_unique<ProjmapEffectMtxSetter>(nullptr, nullptr);
        auto* result = controller.get();
        actor_controllers()[actor].push_back(std::move(controller));
        controller_actors().emplace(result, ProjectionState{actor, smgpc::render::J3dMatrix3x4{{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        }}});
        return result;
    }

    void release_actor_material_ctrl_state(const LiveActor* actor) {
        const auto found = actor_controllers().find(actor);
        if (found == actor_controllers().end()) {
            return;
        }
        for (const auto& controller : found->second) {
            controller_actors().erase(controller.get());
        }
        actor_controllers().erase(found);
    }
}

namespace MR {
    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* actor) {
        return smgpc::compat::create_projmap_effect_mtx_setter(actor);
    }
}
