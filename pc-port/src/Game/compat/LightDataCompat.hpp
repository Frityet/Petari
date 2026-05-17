#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Game/Map/LightDataHolder.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"

namespace smgpc::game {

    class LightDataCompat final {
    public:
        static LightDataCompat &instance();

        void reset();
        [[nodiscard]] AreaLightInfo *area_light_info(const ZoneLightID &zone_id);
        [[nodiscard]] const char *default_area_light_name();

    private:
        bool ensure_loaded();
        void load_current_stage(std::string_view stage_name);

        struct ZoneAreaLight {
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
        std::vector<ZoneAreaLight> _zone_area_lights;
    };

}  // namespace smgpc::game
