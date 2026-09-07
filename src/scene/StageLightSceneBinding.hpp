#pragma once

#include <span>
#include <string_view>

#include "scene/StagePlacementResolver.hpp"

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::scene {

    // Owns the process-wide retail LightData cache for exactly one active
    // scene. Zone identities come from generalized placement-table metadata;
    // the binding resets the cache on teardown so a later scene cannot inherit
    // a child zone or AreaLight row.
    class StageLightSceneBinding final {
    public:
        StageLightSceneBinding(smgpc::runtime::DvdFileSystemService &dvd,
                               std::string_view stage_name,
                               std::span<const StagePlacementTable> tables);
        ~StageLightSceneBinding();

        StageLightSceneBinding(const StageLightSceneBinding &) = delete;
        StageLightSceneBinding &operator=(const StageLightSceneBinding &) = delete;
        StageLightSceneBinding(StageLightSceneBinding &&) = delete;
        StageLightSceneBinding &operator=(StageLightSceneBinding &&) = delete;

    private:
        bool _owns_cache = false;
    };

}  // namespace smgpc::scene
