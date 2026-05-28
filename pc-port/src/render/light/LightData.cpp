#include "render/light/LightData.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

namespace smgpc::render::light {
    namespace {
        constexpr auto cLightDataArchivePath = std::string_view{"ObjectData/LightData.arc"};
        constexpr auto cMainLightDataPath = std::string_view{"lightdata.bcsv"};

        [[nodiscard]] std::string lower_ascii(std::string_view text) {
            auto out = std::string(text);
            std::ranges::transform(out, out.begin(), [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });
            return out;
        }

        [[nodiscard]] std::string zone_light_file_name(std::string_view stage_name) {
            return "light" + lower_ascii(stage_name) + ".bcsv";
        }

        [[nodiscard]] const smgpc::resource::RarcEntry *find_archive_file(const smgpc::resource::RarcArchive &archive, std::string_view path) {
            if (const auto *entry = archive.find(path); entry != nullptr) {
                return entry;
            }

            const auto wanted = lower_ascii(path);
            for (const auto &entry : archive.entries()) {
                if (lower_ascii(entry.path) == wanted) {
                    return &entry;
                }
            }

            return nullptr;
        }

        [[nodiscard]] std::int32_t get_s32_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view field_name, std::int32_t fallback) {
            if (const auto value = table.get_s32(row, field_name); value.has_value()) {
                return *value;
            }
            return fallback;
        }

        [[nodiscard]] std::optional<bool> get_bool(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view field_name) {
            if (const auto value = table.get_s32(row, field_name); value.has_value()) {
                return *value != 0;
            }
            return std::nullopt;
        }

        [[nodiscard]] bool get_bool_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view field_name, bool fallback) {
            if (const auto value = get_bool(table, row, field_name); value.has_value()) {
                return *value;
            }
            return fallback;
        }

        [[nodiscard]] std::uint8_t get_u8_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view field_name, std::uint8_t fallback) {
            if (const auto value = table.get_s32(row, field_name); value.has_value()) {
                return static_cast<std::uint8_t>(*value);
            }
            return fallback;
        }

        [[nodiscard]] float get_float_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view field_name, float fallback) {
            if (const auto value = table.get_float(row, field_name); value.has_value()) {
                return *value;
            }
            return fallback;
        }

        [[nodiscard]] _GXColor get_color_channels(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view prefix) {
            const auto base = std::string(prefix);
            return _GXColor{
                get_u8_or(table, row, base + "R", 0U),
                get_u8_or(table, row, base + "G", 0U),
                get_u8_or(table, row, base + "B", 0U),
                get_u8_or(table, row, base + "A", 0U),
            };
        }

        [[nodiscard]] TVec3f get_position(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view prefix) {
            const auto base = std::string(prefix);
            return TVec3f{
                get_float_or(table, row, base + "PosX", 0.0F),
                get_float_or(table, row, base + "PosY", 0.0F),
                get_float_or(table, row, base + "PosZ", 0.0F),
            };
        }

        [[nodiscard]] bool get_follow_camera(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view prefix) {
            const auto base = std::string(prefix);
            if (const auto value = get_bool(table, row, base + "FollowCamera"); value.has_value()) {
                return *value;
            }
            if (const auto value = get_bool(table, row, base + "FollowCamra"); value.has_value()) {
                return *value;
            }
            return false;
        }

        void read_light_info(const smgpc::resource::BcsvTable &table, std::size_t row, LightInfo &info, std::string_view name) {
            info.mColor = get_color_channels(table, row, std::string(name) + "Color");
            info.mPos = get_position(table, row, name);
            info.mIsFollowCamera = get_follow_camera(table, row, name);
        }

        void read_actor_light_info(const smgpc::resource::BcsvTable &table, std::size_t row, ActorLightInfo &info, std::string_view name) {
            const auto base = std::string(name);
            read_light_info(table, row, info.mInfo0, base + "Light0");
            read_light_info(table, row, info.mInfo1, base + "Light1");
            info.mAlpha2 = get_u8_or(table, row, base + "Alpha2", 0U);
            info.mColor = get_color_channels(table, row, base + "Ambient");
        }

        [[nodiscard]] bool same_area_name(const AreaLightInfo &info, std::string_view name) {
            return info.mAreaLightName != nullptr && name == std::string_view(info.mAreaLightName);
        }

    }  // namespace

    StageLightData &StageLightData::instance() {
        static auto s_instance = StageLightData{};
        return s_instance;
    }

    void StageLightData::reset() {
        _loaded = false;
        _load_failed = false;
        _root_key.clear();
        _stage_key.clear();
        _default_stage_area_light_name.clear();
        _area_light_names.clear();
        _area_lights.clear();
        _zone_area_lights.clear();
    }

    AreaLightInfo *StageLightData::area_light_info(const ZoneLightID &zone_id) {
        if (!ensure_loaded() || _area_lights.empty()) {
            return nullptr;
        }

        auto area_name = std::string_view{};
        if (!_zone_area_lights.empty()) {
            const auto exact = std::ranges::find_if(_zone_area_lights, [&zone_id](const auto &entry) {
                return entry.light_id == zone_id.mLightID;
            });
            if (exact != _zone_area_lights.end()) {
                area_name = exact->area_light_name;
            } else {
                const auto fallback = std::ranges::find_if(_zone_area_lights, [](const auto &entry) {
                    return entry.light_id < 0;
                });
                if (fallback != _zone_area_lights.end()) {
                    area_name = fallback->area_light_name;
                }
            }
        }

        if (area_name.empty()) {
            area_name = _default_stage_area_light_name;
        }

        if (!area_name.empty()) {
            const auto area = std::ranges::find_if(_area_lights, [area_name](const auto &info) {
                return same_area_name(info, area_name);
            });
            if (area != _area_lights.end()) {
                return &*area;
            }
        }

        return &_area_lights.front();
    }

    const char *StageLightData::default_area_light_name() {
        if (!ensure_loaded()) {
            return nullptr;
        }

        if (!_default_stage_area_light_name.empty()) {
            return _default_stage_area_light_name.c_str();
        }

        if (!_area_lights.empty()) {
            return _area_lights.front().mAreaLightName;
        }

        return nullptr;
    }

    bool StageLightData::ensure_loaded() {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return false;
        }

        auto stage_name = std::string(runtime->current_stage_name());
        if (stage_name.empty()) {
            return false;
        }

        const auto root_key = runtime->dvd().root().generic_string();
        if (_loaded && _root_key == root_key && _stage_key == stage_name) {
            return true;
        }
        if (_load_failed && _root_key == root_key && _stage_key == stage_name) {
            return false;
        }

        reset();
        _root_key = root_key;
        _stage_key = stage_name;

        try {
            load_current_stage(stage_name);
            _loaded = true;
            return true;
        } catch (const std::exception &error) {
            _load_failed = true;
#ifndef NDEBUG
            runtime->note_debug_event(std::string("StageLightData failed to load original light data: ") + error.what());
#else
            (void)error;
#endif
            return false;
        }
    }

    void StageLightData::load_current_stage(std::string_view stage_name) {
        auto &runtime = smgpc::runtime::RuntimeContext::instance();
        const auto &archive = runtime.dvd().archive(cLightDataArchivePath);

        const auto *main_entry = find_archive_file(archive, cMainLightDataPath);
        if (main_entry == nullptr) {
            throw std::runtime_error("missing lightdata.bcsv in LightData.arc");
        }

        const auto light_table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*main_entry));
        _area_light_names.reserve(light_table.entry_count());
        _area_lights.reserve(light_table.entry_count());
        for (auto row = std::size_t{}; row < light_table.entry_count(); ++row) {
            auto info = AreaLightInfo{};
            _area_light_names.push_back(light_table.get_string(row, "AreaLightName").value_or(std::string{}));
            info.mAreaLightName = _area_light_names.back().c_str();
            info.mInterpolate = get_s32_or(light_table, row, "Interpolate", -1);
            read_actor_light_info(light_table, row, info.mPlayerLight, "Player");
            read_actor_light_info(light_table, row, info.mStrongLight, "Strong");
            read_actor_light_info(light_table, row, info.mWeakLight, "Weak");
            read_actor_light_info(light_table, row, info.mPlanetLight, "Planet");
            info.mFix = get_bool_or(light_table, row, "Fix", false);
            _area_lights.push_back(info);
        }

        const auto zone_file = zone_light_file_name(stage_name);
        if (const auto *zone_entry = find_archive_file(archive, zone_file); zone_entry != nullptr) {
            const auto zone_table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*zone_entry));
            _zone_area_lights.reserve(zone_table.entry_count());
            for (auto row = std::size_t{}; row < zone_table.entry_count(); ++row) {
                _zone_area_lights.push_back(ZoneAreaLight{
                    .light_id = get_s32_or(zone_table, row, "LightID", -1),
                    .area_light_name = zone_table.get_string(row, "AreaLightName").value_or(std::string{}),
                });
            }
        }

        if (!_zone_area_lights.empty()) {
            const auto fallback = std::ranges::find_if(_zone_area_lights, [](const auto &entry) {
                return entry.light_id < 0;
            });
            _default_stage_area_light_name = fallback != _zone_area_lights.end() ? fallback->area_light_name : _zone_area_lights.front().area_light_name;
        } else if (!_area_light_names.empty()) {
            _default_stage_area_light_name = _area_light_names.front();
        }
    }

}  // namespace smgpc::render::light
