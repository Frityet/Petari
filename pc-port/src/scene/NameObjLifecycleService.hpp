#pragma once

#include "scene/nameobj/NameObjFactory.hpp"

#include <memory>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    struct StagePlacementObject;

    class NameObjLifecycleService final {
    public:
        explicit NameObjLifecycleService(smgpc::runtime::RuntimeContext &runtime);
        ~NameObjLifecycleService();

        NameObjLifecycleService(const NameObjLifecycleService &) = delete;
        NameObjLifecycleService &operator=(const NameObjLifecycleService &) = delete;

        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> preload_archives(std::string_view object_name);
        [[nodiscard]] std::unique_ptr<NameObj> construct(std::string_view object_name, std::string_view actor_name);
        void init(NameObj &object, const StagePlacementObject *placement);
        void init_after_placement(NameObj &object);
        void appear(NameObj &object);
        void destroy(NameObj &object);

    private:
        smgpc::runtime::RuntimeContext &_runtime;
    };

}  // namespace smgpc::scene
