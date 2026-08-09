#include "scene/StageLightSceneBinding.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "render/light/LightData.hpp"
#include "runtime/RuntimeServices.hpp"

namespace smgpc::scene {
    namespace {
        StageLightSceneBinding *sActiveBinding = nullptr;
    }

    StageLightSceneBinding::StageLightSceneBinding(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
        std::span<const StagePlacementTable> tables) {
        if (stage_name.empty()) {
            throw std::invalid_argument("stage-light scene binding requires an authored stage name");
        }
        if (sActiveBinding != nullptr) {
            throw std::logic_error("only one scene may own the stage-light cache at a time");
        }

        sActiveBinding = this;
        auto &data = smgpc::render::light::StageLightData::instance();
        try {
            auto zones = std::vector<smgpc::render::light::StageLightZone>{};
            zones.reserve(tables.size() + 1U);
            zones.push_back({.zone_id = 0, .zone_name = std::string(stage_name)});
            for (const auto &table : tables) {
                if (table.zone_id < 0 || table.zone_name.empty()) {
                    continue;
                }
                zones.push_back({.zone_id = table.zone_id, .zone_name = table.zone_name});
            }

            data.reset();
            data.configure_stage_zones(zones);
            data.load_stage(dvd, stage_name);
            _owns_cache = true;
        } catch (...) {
            data.reset();
            sActiveBinding = nullptr;
            throw;
        }
    }

    StageLightSceneBinding::~StageLightSceneBinding() {
        if (_owns_cache && sActiveBinding == this) {
            smgpc::render::light::StageLightData::instance().reset();
            sActiveBinding = nullptr;
        }
    }

}  // namespace smgpc::scene
