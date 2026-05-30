#include "scene/NameObjLifecycleService.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/StagePlacementResolver.hpp"

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
#endif

    }  // namespace

    NameObjLifecycleService::NameObjLifecycleService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    NameObjLifecycleService::~NameObjLifecycleService() = default;

    std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> NameObjLifecycleService::preload_archives(std::string_view object_name) {
        auto requests = smgpc::scene::nameobj::preload_name_obj_archives(_runtime.dvd(), object_name);
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

    std::unique_ptr<NameObj> NameObjLifecycleService::construct(std::string_view object_name, std::string_view actor_name) {
        auto object = smgpc::scene::nameobj::create_name_obj(_runtime.dvd(), object_name, actor_name);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("name_obj_lifecycle", "construct",
                                           "object=" + std::string(object_name) + ";actor=" + std::string(actor_name));
#endif
        return object;
    }

    void NameObjLifecycleService::init(NameObj &object, const StagePlacementObject *placement) {
        if (placement != nullptr) {
            const auto placement_iter = JMapInfoIter(&placement->jmap_info, placement->jmap_entry_index);
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event("name_obj_lifecycle", "init_from_placement",
                                               "object=" + object_name(object) + ";stage=" + placement->stage_name +
                                                   ";zone=" + placement->zone_name + ";table=" + placement->table_path +
                                                   ";row=" + std::to_string(placement->jmap_entry_index) +
                                                   ";l_id=" + std::to_string(placement->l_id));
#endif
            object.init(placement_iter);
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
    }

}  // namespace smgpc::scene
