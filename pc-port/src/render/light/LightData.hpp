#pragma once

#include <string>
#include <string_view>
#include <span>
#include <vector>

#include "Game/Map/LightDataHolder.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::render::light {

    struct StageLightZone final {
        s32 zone_id = -1;
        std::string zone_name;

        bool operator==(const StageLightZone &) const = default;
    };

    class StageLightData final {
    public:
        static StageLightData &instance();

        void reset();
        void configure_stage_zones(std::span<const StageLightZone> zones);
        void load_stage(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name);
        [[nodiscard]] std::span<const StageLightZone> stage_zones() const;
        [[nodiscard]] AreaLightInfo *area_light_info(const ZoneLightID &zone_id);
        [[nodiscard]] const char *default_area_light_name();

    private:
        void clear_loaded_data();
        bool ensure_loaded();
        void load_current_stage(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name);

        struct ZoneAreaLight {
            s32 zone_id = -1;
            s32 light_id = -1;
            std::string area_light_name;
        };

        bool _loaded = false;
        bool _load_failed = false;
        std::string _root_key;
        std::string _stage_key;
        std::string _default_stage_area_light_name;
        std::vector<std::string> _area_light_names;
        std::vector<AreaLightInfo> _area_lights;
        std::vector<StageLightZone> _stage_zones;
        std::vector<ZoneAreaLight> _zone_area_lights;
    };

}  // namespace smgpc::render::light
