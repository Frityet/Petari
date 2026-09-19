#include <aurora/exception.hpp>
#include "scene/StageLightSceneBinding.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "render/light/LightData.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "runtime/RuntimeServices.hpp"

namespace smgpc::scene {
    namespace {
        StageLightSceneBinding *sActiveBinding = nullptr;

        void append_zones(const StageDataHolder &holder,
                          std::vector<smgpc::render::light::StageLightZone> &zones) {
            if (holder._A8 == nullptr || holder.mZoneID < 0 ||
                holder.mStageDataHolderCount < 0 || static_cast<std::size_t>(holder.mStageDataHolderCount) > std::size(holder.mStageDataArray)) {
                aurora::throw_host_exception<std::logic_error>("stage-light binding requires initialized original stage holders");
            }
            zones.push_back({.zone_id = holder.mZoneID, .zone_name = holder._A8});
            for (s32 index = 0; index < holder.mStageDataHolderCount; ++index) {
                if (holder.mStageDataArray[index] == nullptr)
                    aurora::throw_host_exception<std::logic_error>("stage-light binding encountered an absent authored child holder");
                append_zones(*holder.mStageDataArray[index], zones);
            }
        }
    }

    StageLightSceneBinding::StageLightSceneBinding(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
        std::span<const StagePlacementTable> tables) {
        auto zones = std::vector<smgpc::render::light::StageLightZone>{};
        zones.reserve(tables.size() + 1U);
        zones.push_back({.zone_id = 0, .zone_name = std::string(stage_name)});
        for (const auto &table : tables) {
            if (table.zone_id < 0 || table.zone_name.empty()) continue;
            zones.push_back({.zone_id = table.zone_id, .zone_name = table.zone_name});
        }
        initialize(dvd, stage_name, zones);
    }

    StageLightSceneBinding::StageLightSceneBinding(
        smgpc::runtime::DvdFileSystemService &dvd, const StageDataHolder &stage) {
        auto zones = std::vector<smgpc::render::light::StageLightZone>{};
        append_zones(stage, zones);
        initialize(dvd, stage._A8, zones);
    }

    void StageLightSceneBinding::initialize(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
        std::span<const smgpc::render::light::StageLightZone> zones) {
        if (stage_name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>("stage-light scene binding requires an authored stage name");
        }
        if (sActiveBinding != nullptr) {
            aurora::throw_host_exception<std::logic_error>("only one scene may own the stage-light cache at a time");
        }

        sActiveBinding = this;
        auto &data = smgpc::render::light::StageLightData::instance();
        try {
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
