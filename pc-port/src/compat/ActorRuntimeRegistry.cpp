#include "compat/ActorRuntimeRegistry.hpp"

#include <unordered_map>
#include <unordered_set>

namespace {
    auto& actor_binders() {
        static auto actors = std::unordered_set<const LiveActor*>{};
        return actors;
    }

    auto& actor_models() {
        static auto models = std::unordered_map<const LiveActor*, smgpc::render::live_actor::LiveActorModel*>{};
        return models;
    }
}  // namespace

namespace smgpc::compat {
    void register_actor_binder(const LiveActor* actor) {
        if (actor != nullptr) {
            actor_binders().insert(actor);
        }
    }

    bool has_actor_binder(const LiveActor* actor) {
        return actor != nullptr && actor_binders().contains(actor);
    }

    void release_actor_binder_state(const LiveActor* actor) {
        actor_binders().erase(actor);
    }

    void register_actor_model(const LiveActor* actor, smgpc::render::live_actor::LiveActorModel* model) {
        if (actor != nullptr) {
            actor_models().insert_or_assign(actor, model);
        }
    }

    smgpc::render::live_actor::LiveActorModel* actor_model(const LiveActor* actor) {
        const auto found = actor_models().find(actor);
        return found != actor_models().end() ? found->second : nullptr;
    }

    void release_actor_model_state(const LiveActor* actor) {
        actor_models().erase(actor);
    }

    void release_actor_runtime_state(const LiveActor* actor) {
        release_actor_binder_state(actor);
        release_actor_model_state(actor);
        release_talk_runtime_state(actor);
        release_demo_runtime_state(actor);
        release_star_piece_runtime_state(actor);
    }
}  // namespace smgpc::compat
