#include "compat/CollisionPartsCompat.hpp"
#include "compat/CollisionDirectorOwnership.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/KCollisionResource.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <array>
#include <cstdio>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {
    struct PartsDeleter {
        void operator()(CollisionParts* parts) const noexcept {
            if (parts == nullptr) return;
            delete parts->mServer->mapInfo;
            delete parts->mServer;
            delete parts;
        }
    };

    struct ActorCollisionPartsState {
        std::shared_ptr<const smgpc::compat::ResourceArchiveOwner> resource_owner;
        SceneObjHolder* scene_holder = nullptr;
        smgpc::scene::StageCollisionService* service = nullptr;
        std::uint64_t service_generation = 0;
        std::span<const std::uint8_t> kcl;
        std::span<const std::uint8_t> attributes;
        std::string resource_name;
        std::string source;
        std::string attributes_source;
        std::unique_ptr<smgpc::resource::KCollisionResource> decoded;
        std::unique_ptr<CollisionParts, PartsDeleter> parts;
        std::shared_ptr<smgpc::scene::StageCollisionRegistrationState> registration;
        std::array<float, 12> published_current;
        std::array<float, 12> published_previous;

        ~ActorCollisionPartsState() {
            if (registration) registration->release_owner();
            // The original zone has a borrowed pointer. Remove it while its
            // scene is available; whole-scene teardown drops the zones next.
            if (parts && parts->_CC && smgpc::scene::current_scene_obj_holder() == scene_holder &&
                scene_holder->isExist(SceneObj_CollisionDirector)) {
                MR::invalidateCollisionParts(parts.get());
            }
        }
    };

    auto& actor_collision_parts() {
        static std::unordered_map<const LiveActor*, std::vector<std::unique_ptr<ActorCollisionPartsState>>> states;
        return states;
    }

    std::array<float, 12> copy_matrix(const TPos3f& matrix) {
        std::array<float, 12> result;
        for (std::size_t row = 0; row < 3; ++row)
            for (std::size_t col = 0; col < 4; ++col) result[row * 4 + col] = matrix.mMtx[row][col];
        return result;
    }

    ActorCollisionPartsState* find_state(const CollisionParts& parts) {
        for (auto& [actor, entries] : actor_collision_parts()) {
            for (auto& entry : entries) if (entry->parts.get() == &parts) return entry.get();
        }
        return nullptr;
    }
}

namespace smgpc::compat {
    CollisionParts* create_collision_parts(ResourceHolder* resources, const char* name,
                                           HitSensor* sensor, const TPos3f& matrix,
                                           int scale_type, s32 category) {
        const aurora::allocation::HostAllocationScope host;
        auto* collision = scene::StageCollisionService::active();
        auto* resource_service = ResourceHolderService::active();
        auto* holder = scene::current_scene_obj_holder();
        if (!resources || !name || !sensor || !sensor->mHost || !collision || !resource_service || !holder) {
            aurora::throw_host_exception<std::logic_error>("CollisionParts requires its actor, resources, scene and collision owners.");
        }
        if (category != 0) collision = &scene::current_collision_director_ownership()->category_service(category);
        if (scale_type < MR::CollisionScaleType_AutoEqualScale || scale_type > MR::CollisionScaleType_Unk2) {
            aurora::throw_host_exception<std::invalid_argument>("CollisionParts scale policy is outside the original enum.");
        }
        const auto placement_zone_id = MR::getCurrentPlacementZoneId();
        if (placement_zone_id < 0 || placement_zone_id >= MR::getZoneNum() || MR::getZoneNum() > 32) {
            aurora::throw_host_exception<std::logic_error>("CollisionParts requires a valid original placement zone.");
        }
        const auto& backing = resource_service->backing(*resources);
        // Retail resource lookup uses two 0x80-byte filename buffers.
        char kcl_name[0x80];
        char attributes_name[0x80];
        std::snprintf(kcl_name, sizeof(kcl_name), "%s.kcl", name);
        std::snprintf(attributes_name, sizeof(attributes_name), "%s.pa", name);
        const auto* kcl_entry = backing.archive().find_resource(kcl_name);
        const auto* attributes_entry = backing.archive().find_resource(attributes_name);
        if (!kcl_entry) aurora::throw_host_exception<std::runtime_error>("Required CollisionParts KCL is unavailable: " + std::string(kcl_name));
        auto state = std::make_unique<ActorCollisionPartsState>();
        state->resource_owner = resource_service->retain(*resources);
        state->scene_holder = holder;
        state->service = collision;
        state->service_generation = collision->generation();
        state->resource_name = name;
        state->source = backing.resolved_path().generic_string() + ":/" + kcl_entry->path;
        state->kcl = backing.archive().file_data(*kcl_entry);
        if (attributes_entry) {
            state->attributes = backing.archive().file_data(*attributes_entry);
            state->attributes_source = backing.resolved_path().generic_string() + ":/" + attributes_entry->path;
        }
        state->decoded = std::make_unique<resource::KCollisionResource>(state->kcl, state->attributes);
        {
            const aurora::allocation::ClientAllocationScope client;
            if (MR::createSceneObj(SceneObj_CollisionDirector) == nullptr) {
                aurora::throw_host_exception<std::logic_error>("CollisionParts requires its original CollisionDirector.");
            }
            state->parts.reset(new CollisionParts());
            auto* data = state->decoded->native_file();
            auto* attrs = state->decoded->attributes_data();
            switch (scale_type) {
            case MR::CollisionScaleType_AutoEqualScale:
                state->parts->initWithAutoEqualScale(matrix, sensor, data, attrs, category, false);
                break;
            case MR::CollisionScaleType_NotUsingScale:
                state->parts->initWithNotUsingScale(matrix, sensor, data, attrs, category, false);
                break;
            case MR::CollisionScaleType_Unk2:
                state->parts->init(matrix, sensor, data, attrs, category, false);
                break;
            }
        }
        state->registration = std::make_shared<scene::StageCollisionRegistrationState>(nullptr, state->parts.get());
        state->registration->set_enabled(false);
        state->published_current = copy_matrix(state->parts->mBaseMatrix);
        state->published_previous = copy_matrix(state->parts->mPrevBaseMatrix);
        const auto result = collision->register_kcl(state->kcl, state->published_current, state->source,
            state->registration, state->attributes, sensor, placement_zone_id);
        if (!result.accepted) aurora::throw_host_exception<std::runtime_error>("Required CollisionParts KCL is malformed: " + state->source);
        if (category != 0) collision->build();
        auto* result_parts = state->parts.get();
        actor_collision_parts()[sensor->mHost].push_back(std::move(state));
        return result_parts;
    }

    scene::StageCollisionService* collision_service_for_parts(const CollisionParts* parts) noexcept {
        if (!parts) return nullptr;
        // Address comparison does not dereference a possibly retired borrower.
        for (auto& [actor, entries] : actor_collision_parts()) {
            for (auto& state : entries) if (state->parts.get() == parts) return state->service;
        }
        return nullptr;
    }

    void publish_collision_parts(CollisionParts& parts) {
        const aurora::allocation::HostAllocationScope host;
        auto* state = find_state(parts);
        // resetAllMtx is also called during init, before native publication.
        if (!state) return;
        if (scene::current_scene_obj_holder() != state->scene_holder || state->service->generation() != state->service_generation) {
            aurora::throw_host_exception<std::logic_error>("CollisionParts matrix publication requires its live collision owner.");
        }
        const auto current = copy_matrix(parts.mBaseMatrix);
        const auto previous = copy_matrix(parts.mPrevBaseMatrix);
        if (current == state->published_current && previous == state->published_previous) return;
        state->service->update_registered_transform(*state->registration, current, previous);
        state->published_current = current;
        state->published_previous = previous;
    }

    void publish_collision_parts_membership(CollisionParts& parts, bool enabled) {
        const aurora::allocation::HostAllocationScope host;
        if (auto* state = find_state(parts)) state->registration->set_enabled(enabled);
    }

    bool has_actor_collision_parts(const LiveActor* actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return actor && found != actor_collision_parts().end() && !found->second.empty();
    }
    std::size_t actor_collision_parts_count(const LiveActor* actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return actor && found != actor_collision_parts().end() ? found->second.size() : 0;
    }
    std::string_view actor_collision_parts_source(const LiveActor* actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return found != actor_collision_parts().end() && !found->second.empty() ? found->second.front()->source : std::string_view{};
    }
    std::vector<ActorCollisionPartsResource> actor_collision_parts_resources(const LiveActor* actor) {
        const aurora::allocation::HostAllocationScope host;
        std::vector<ActorCollisionPartsResource> result;
        const auto found = actor_collision_parts().find(actor);
        if (found == actor_collision_parts().end()) return result;
        for (const auto& state : found->second) {
            result.push_back({state->resource_name, state->source, state->attributes_source,
                              state->kcl.size(), state->attributes.size(), state->parts->_D8});
        }
        return result;
    }
    void release_actor_collision_parts(const LiveActor* actor) noexcept {
        const aurora::allocation::HostAllocationScope host;
        const auto found = actor_collision_parts().find(actor);
        if (found == actor_collision_parts().end()) return;
        // Detach the entries before destroying them: original invalidation
        // publishes membership and must not traverse an erasing container.
        auto entries = std::move(found->second);
        actor_collision_parts().erase(found);
        const_cast<LiveActor*>(actor)->mCollisionParts = nullptr;
    }
    void release_scene_collision_parts(const SceneObjHolder* holder) noexcept {
        const aurora::allocation::HostAllocationScope host;
        for (auto it = actor_collision_parts().begin(); it != actor_collision_parts().end();) {
            auto* actor = it->first;
            bool belongs = !it->second.empty() && it->second.front()->scene_holder == holder;
            ++it;
            if (belongs) release_actor_collision_parts(actor);
        }
    }
}
