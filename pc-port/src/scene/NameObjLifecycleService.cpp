#include "scene/NameObjLifecycleService.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/PlacementZoneNameScope.hpp"

#include <string>

namespace smgpc::scene {
    namespace {

#ifndef NDEBUG
        [[nodiscard]] std::string archive_kind_name(smgpc::scene::nameobj::NameObjArchiveKind kind) {
            switch (kind) {
            case smgpc::scene::nameobj::NameObjArchiveKind::Object:
                return "object";
            case smgpc::scene::nameobj::NameObjArchiveKind::Layout:
                return "layout";
            case smgpc::scene::nameobj::NameObjArchiveKind::Missing:
                return "missing";
            }
            return "unknown";
        }

        [[nodiscard]] std::string bool_text(bool value) {
            return value ? "true" : "false";
        }

        [[nodiscard]] std::string object_name(const NameObj &object) {
            return object.getName() != nullptr ? object.getName() : "";
        }

        [[nodiscard]] std::string_view placement_source_name(NameObjPlacementSource source) {
            switch (source) {
            case NameObjPlacementSource::StagePlacement:
                return "placement";
            case NameObjPlacementSource::StageStart:
                return "start";
            }
            return "unknown";
        }
#endif

        void require_valid_placement_context(const NameObjPlacementContext &placement) {
            if (!placement.iter.isValid() || placement.row != placement.iter.mIndex) {
                throw std::logic_error("NameObj placement context does not own a valid retail JMap row.");
            }
        }

    }  // namespace

    NameObjLifecycleService::NameObjLifecycleService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    NameObjLifecycleService::~NameObjLifecycleService() = default;

    std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> NameObjLifecycleService::preload_archives(
        std::string_view object_name, const NameObjPlacementContext *placement) {
        if (placement != nullptr) {
            require_valid_placement_context(*placement);
        }
        auto requests = smgpc::scene::nameobj::preload_name_obj_archives(_runtime.dvd(), object_name,
                                                                         placement != nullptr ? &placement->iter : nullptr);
#ifndef NDEBUG
        for (const auto &request : requests) {
            _runtime.emit_semantic_trace_event("name_obj_lifecycle", "archive_request",
                                               "object=" + std::string(object_name) + ";archive=" + request.archive_name +
                                                   ";kind=" + archive_kind_name(request.kind) + ";loaded=" + bool_text(request.loaded) +
                                                   ";disc_path=" + request.disc_path);
        }
#endif
        return requests;
    }

    std::unique_ptr<NameObj> NameObjLifecycleService::construct(std::string_view object_name, const char *actor_name) {
        auto object = smgpc::scene::nameobj::create_name_obj(_runtime.dvd(), object_name, actor_name);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "construct",
                                           "object=" + std::string(object_name) + ";actor=" +
                                               (actor_name != nullptr ? std::string(actor_name) : "<absent>"));
#endif
        return object;
    }

    std::unique_ptr<NameObj> NameObjLifecycleService::construct_and_init(
        std::string_view object_name, const char *actor_name,
        const NameObjPlacementContext *placement) {
        if (placement == nullptr) {
            auto object = construct(object_name, actor_name);
            try {
                init(*object, nullptr);
            } catch (...) {
                destroy(*object);
                throw;
            }
            return object;
        }

        require_valid_placement_context(*placement);
        auto zone_scope = smgpc::scene::PlacementZoneNameScope(
            MR::getPlacedZoneId(placement->iter), placement->zone_name);
        auto object = construct(object_name, actor_name);
        try {
            init(*object, placement);
        } catch (...) {
            destroy(*object);
            throw;
        }
        return object;
    }

    void NameObjLifecycleService::init(NameObj &object, const NameObjPlacementContext *placement) {
        if (placement != nullptr) {
            require_valid_placement_context(*placement);
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event(
                "name_obj_lifecycle", "init_from_jmap",
                "object=" + object_name(object) + ";source=" +
                    std::string(placement_source_name(placement->source)) + ";stage=" +
                    std::string(placement->stage_name) + ";zone=" + std::string(placement->zone_name) +
                    ";table=" + std::string(placement->table_path) + ";row=" +
                    std::to_string(placement->row) + ";local_id=" + std::to_string(placement->local_id));
#endif
            object.init(placement->iter);
            return;
        }

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "init_without_iter", "object=" + object_name(object));
#endif
        object.initWithoutIter();
    }

    void NameObjLifecycleService::init_after_placement(NameObj &object) {
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "init_after_placement", "object=" + object_name(object));
#endif
        object.initAfterPlacement();
    }

    void NameObjLifecycleService::appear(NameObj &object) {
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "appear", "object=" + object_name(object));
#endif
        if (auto *layout_actor = dynamic_cast<LayoutActor *>(&object)) {
            layout_actor->appear();
        } else if (auto *live_actor = dynamic_cast<LiveActor *>(&object)) {
            live_actor->appear();
        }
    }

    void NameObjLifecycleService::destroy(NameObj &object) {
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "destroy", "object=" + object_name(object));
#endif
        if (auto *live_actor = dynamic_cast<LiveActor *>(&object)) {
            smgpc::compat::release_actor_runtime_state(live_actor);
        }
    }

}  // namespace smgpc::scene
