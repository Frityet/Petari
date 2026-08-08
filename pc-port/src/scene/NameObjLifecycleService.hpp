#pragma once

#include "Game/Util/JMapInfo.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <memory>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    enum class NameObjPlacementSource {
        StagePlacement,
        StageStart,
    };

    struct NameObjPlacementContext {
        JMapInfoIter iter;
        NameObjPlacementSource source = NameObjPlacementSource::StagePlacement;
        std::string_view stage_name;
        std::string_view zone_name;
        std::string_view table_path;
        s32 row = -1;
        s32 local_id = -1;
    };

    class NameObjLifecycleService final {
    public:
        explicit NameObjLifecycleService(smgpc::runtime::RuntimeContext &runtime);
        ~NameObjLifecycleService();

        NameObjLifecycleService(const NameObjLifecycleService &) = delete;
        NameObjLifecycleService &operator=(const NameObjLifecycleService &) = delete;

        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> preload_archives(std::string_view object_name,
                                                                                  const NameObjPlacementContext *placement = nullptr);
        [[nodiscard]] std::unique_ptr<NameObj> construct(std::string_view object_name, const char *actor_name);
        [[nodiscard]] std::unique_ptr<NameObj> construct_and_init(
            std::string_view object_name, const char *actor_name,
            const NameObjPlacementContext *placement);
        void init(NameObj &object, const NameObjPlacementContext *placement);
        void init_after_placement(NameObj &object);
        void appear(NameObj &object);
        void destroy(NameObj &object);

    private:
        smgpc::runtime::RuntimeContext &_runtime;
    };

}  // namespace smgpc::scene
