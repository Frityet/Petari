#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/Map/OceanHomeMapCtrl.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "render/effects/EffectResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    [[nodiscard]] smgpc::render::live_actor::LiveActorModel &
    require_planet_model(const LiveActor *actor) {
        auto *model = smgpc::compat::actor_model(actor);
        if (actor == nullptr || model == nullptr || model->model_arc_name().empty()) {
            throw std::logic_error("PlanetMap runtime support requires a real actor model.");
        }
        return *model;
    }

    [[nodiscard]] ResourceHolder *require_actor_resource_holder(const LiveActor *actor) {
        const auto &model = require_planet_model(actor);
        auto *service = smgpc::compat::ResourceHolderService::active();
        if (service == nullptr) {
            throw std::logic_error("PlanetMap runtime support requires a scene-owned ResourceHolder service.");
        }
        return service->create_and_add(std::string(model.model_arc_name()) + ".arc");
    }

    [[nodiscard]] bool has_collision_resource(const LiveActor *actor,
                                              std::string_view resource_name) {
        if (actor == nullptr || resource_name.empty()) {
            return false;
        }
        const auto *holder = require_actor_resource_holder(actor);
        return smgpc::compat::ResourceHolderService::active()->backing(*holder).archive().find_resource(std::string(resource_name) + ".kcl") != nullptr;
    }

    CollisionParts *try_register_auxiliary_collision(LiveActor *actor,
                                                     std::string_view resource_name,
                                                     HitSensor *sensor) {
        if (!has_collision_resource(actor, resource_name)) {
            return nullptr;
        }
        auto *holder = require_actor_resource_holder(actor);
        const auto name = std::string(resource_name);
        MR::initCollisionPartsFromResourceHolder(actor, name.c_str(), sensor, holder, nullptr);

        // CollisionParts is represented by the scene-owned generalized KCL
        // registration on PC. PlanetMap only consumes the side effect; the
        // external resource list remains the truthful inspection boundary.
        return nullptr;
    }

    [[noreturn]] void reject_optional_planet_model(std::string_view kind) {
        throw std::logic_error("PlanetMap " + std::string(kind) +
                               " submodel reached the zero-optional runtime tranche.");
    }

}  // namespace

namespace MR {

    bool isExistIndirectTexture(const LiveActor *actor) {
        return require_planet_model(actor).has_indirect_texture();
    }

    void initCollisionParts(LiveActor *actor, const char *resource_name,
                            HitSensor *sensor, MtxPtr matrix) {
        if (actor == nullptr || resource_name == nullptr || sensor == nullptr) {
            throw std::invalid_argument(
                "Planet CollisionParts requires an actor, exact resource name, and sensor.");
        }
        MR::initCollisionPartsFromResourceHolder(
            actor, resource_name, sensor, require_actor_resource_holder(actor), matrix);
    }

    CollisionParts *tryCreateCollisionMoveLimit(LiveActor *actor, HitSensor *sensor) {
        return try_register_auxiliary_collision(actor, "MoveLimit", sensor);
    }

    CollisionParts *tryCreateCollisionWaterSurface(LiveActor *actor, HitSensor *sensor) {
        return try_register_auxiliary_collision(actor, "WaterSurface", sensor);
    }

    bool isExistAnim(const LiveActor *actor, const char *name) {
        if (actor == nullptr || name == nullptr) {
            return false;
        }
        auto &model = require_planet_model(actor);
        if (model.hasBck(name, {}) || model.hasBtk(name) || model.hasBrk(name) ||
            model.hasBtp(name)) {
            return true;
        }

        // LiveActorModel currently evaluates BCK/BTK/BRK/BTP. Preserve the
        // retail all-animation existence test for BPK/BVA without claiming
        // playback support for those optional formats.
        const auto *holder = require_actor_resource_holder(actor);
        return smgpc::compat::ResourceHolderService::active()->backing(*holder).archive().find_resource(std::string(name) + ".bpk") != nullptr ||
               smgpc::compat::ResourceHolderService::active()->backing(*holder).archive().find_resource(std::string(name) + ".bva") != nullptr;
    }

    PartsModel *createWaterModel(LiveActor *actor, MtxPtr) {
        const auto model_name = require_planet_model(actor).model_arc_name();
        if (!MR::isExistSubModel(model_name.data(), "Water")) {
            return nullptr;
        }
        reject_optional_planet_model("Water");
    }

    PartsModel *createIndirectPlanetModel(LiveActor *actor, MtxPtr) {
        const auto model_name = require_planet_model(actor).model_arc_name();
        if (!MR::isExistSubModel(model_name.data(), "Indirect")) {
            return nullptr;
        }
        reject_optional_planet_model("Indirect");
    }

    ModelObj *createModelObjBloomModel(const char *, const char *, MtxPtr) {
        reject_optional_planet_model("Bloom");
    }

    LodCtrl *createLodCtrlPlanet(LiveActor *actor, const JMapInfoIter &iter,
                                 f32 far_clip, s32 low_movement_type) {
        if (actor == nullptr) {
            throw std::invalid_argument("Planet LodCtrl requires a LiveActor.");
        }

        auto lod = std::make_unique<LodCtrl>(actor, iter);
        lod->createLodModel(MR::DrawBufferType_PlanetLow, low_movement_type,
                            MR::DrawBufferType_Sky);
        if (lod->_10 != nullptr || lod->_14 != nullptr) {
            reject_optional_planet_model("LOD");
        }
        lod->setDistanceToMiddleAndLow(5000.0F, 10000.0F);
        lod->setFarClipping(far_clip);

        auto *result = lod.release();
        smgpc::compat::adopt_actor_lod_ctrl(actor, result);
        return result;
    }

    void initFurPlanet(LiveActor *) {
        throw std::logic_error(
            "Fur PlanetMap reached the ordinary zero-optional runtime tranche.");
    }

    bool isRegisteredEffect(const LiveActor *actor, const char *effect_name) {
        if (actor == nullptr || effect_name == nullptr || effect_name[0] == '\0') {
            return false;
        }
        const auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return false;
        }
        const auto keeper = runtime->effects().registered_keeper(actor->getName(), actor);
        const auto *library = runtime->effects().resource_library();
        if (!keeper.has_value() || library == nullptr) {
            return false;
        }
        if (!keeper->resource_group_name.empty() &&
            !library->resolve_auto_effect(keeper->resource_group_name, effect_name).empty()) {
            return true;
        }
        return !library->resolve_effect_request(effect_name).empty();
    }

    bool isExistJoint(const LiveActor *actor, const char *joint_name) {
        if (actor == nullptr || joint_name == nullptr) {
            return false;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto &model = require_planet_model(actor);
        return model.joint_world_matrix(
                   joint_name, smgpc::compat::actor_base_matrix(actor),
                   runtime != nullptr ? runtime->frame_index() : 0U) != nullptr;
    }

    void calcModelBoundingRadius(f32 *radius, const LiveActor *actor) {
        if (radius == nullptr) {
            throw std::invalid_argument("Model bounding-radius output is null.");
        }
        const auto value = require_planet_model(actor).model_bounding_radius();
        if (!value.has_value()) {
            throw std::logic_error("PlanetMap model has no parsed bounding radius.");
        }
        *radius = *value;
    }

    bool isExistCollisionResource(const LiveActor *actor, const char *resource_name) {
        return resource_name != nullptr && has_collision_resource(actor, resource_name);
    }

    bool isExistSubModel(const char *model_name, const char *suffix) {
        if (model_name == nullptr || suffix == nullptr) {
            return false;
        }
        const auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            throw std::logic_error("Submodel lookup requires the active DVD runtime.");
        }
        return runtime->find_object_archive(std::string(model_name) + suffix).has_value();
    }

    bool isExistEffectTexMtx(LiveActor *actor) {
        return require_planet_model(actor).has_effect_texture_matrix();
    }

}  // namespace MR

namespace OceanHomeMapFunction {

    void tryEntryOceanHomeMap(PlanetMap *planet) {
        if (planet == nullptr || planet->mName == nullptr) {
            return;
        }
        const auto name = std::string_view(planet->mName);
        if (name == "海洋ホーム惑星" || name == "オーシャンリング惑星") {
            throw std::logic_error(
                "OceanHome PlanetMap control is unavailable in the ordinary planet tranche.");
        }
    }

}  // namespace OceanHomeMapFunction
