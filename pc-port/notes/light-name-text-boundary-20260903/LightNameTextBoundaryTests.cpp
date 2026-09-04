#include "render/light/LightData.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"
#include "Game/Map/LightFunction.hpp"
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::string lower_ascii(std::string_view text) {
    std::string out(text);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}
bool non_ascii(std::string_view value) {
    return std::ranges::any_of(value, [](unsigned char c) { return c >= 0x80; });
}
void exercise(smgpc::runtime::DvdFileSystemService& dvd) {
    using namespace smgpc;
    auto& lights = render::light::StageLightData::instance();
    struct Reset { ~Reset() { render::light::StageLightData::instance().reset(); } } reset;
    const auto& archive = dvd.archive("ObjectData/LightData.arc");
    const resource::RarcEntry* main_entry = nullptr;
    for (const auto& entry : archive.entries())
        if (lower_ascii(entry.name) == "lightdata.bcsv") main_entry = &entry;
    require(main_entry != nullptr, "actual LightData.arc contains its main table");
    const auto main = resource::BcsvTable::from_bytes(archive.file_data(*main_entry));
    std::map<std::string, std::size_t> main_rows;
    std::size_t main_non_ascii = 0;
    for (std::size_t row = 0; row < main.entry_count(); ++row) {
        auto name = main.get_string(row, "AreaLightName");
        require(name.has_value(), "every actual main light row has an authored name");
        main_non_ascii += non_ascii(*name);
        main_rows.try_emplace(*name, row);
    }
    require(main_non_ascii > 0, "real fixture includes CP932 Japanese identities");

    std::size_t zone_tables = 0, zone_rows = 0, non_ascii_rows = 0, unmatched_rows = 0;
    std::set<std::string> verified_main_names;
    for (const auto& entry : archive.entries()) {
        const auto file = lower_ascii(entry.name);
        if (file == "lightdata.bcsv" || !file.starts_with("light") || !file.ends_with(".bcsv")) continue;
        const auto table = resource::BcsvTable::from_bytes(archive.file_data(entry));
        if (!table.field_index("LightID")) continue;
        const auto zone_name = file.substr(5, file.size() - 10);
        const auto zones = std::array{render::light::StageLightZone{0, zone_name}};
        lights.configure_stage_zones(zones);
        lights.load_stage(dvd, zone_name);
        std::map<s32, std::string> first_zone_rows;
        for (std::size_t row = 0; row < table.entry_count(); ++row) {
            const auto id = table.get_s32(row, "LightID").value_or(-1);
            const auto raw = table.get_string(row, "AreaLightName").value_or(std::string{});
            first_zone_rows.try_emplace(id, raw);
            ++zone_rows;
        }
        for (const auto& [id, raw] : first_zone_rows) {
            ZoneLightID key;
            key._0 = 0;
            key.mLightID = id;
            auto* published = LightFunction::getAreaLightInfo(key);
            require(published == lights.area_light_info(key), "Game accessor publishes the retained service object");
            require(published && published->mAreaLightName, "actual authored zone resolves an AreaLight object");
            const auto found = main_rows.find(raw);
            if (found == main_rows.end()) {
                ++unmatched_rows;
                require(std::string_view(published->mAreaLightName) ==
                            main.get_string(0, "AreaLightName").value(),
                        "unmatched raw names retain the existing first-main-row fallback");
                const auto display = resource::decode_cp932(raw);
                const auto decoded_alias = std::ranges::any_of(main_rows, [&](const auto& candidate) {
                    return resource::decode_cp932(candidate.first) == display;
                });
                std::cout << "[note] unmatched zone_table=" << entry.name << " light_id=" << id
                          << " display=" << display << " decoded_main_alias=" << decoded_alias << '\n';
                continue;
            }
            const auto source_name = main.get_string(found->second, "AreaLightName");
            require(std::string_view(published->mAreaLightName) == *source_name,
                    "Game light identity preserves exact authored CP932 bytes");
            require(std::string_view(published->mAreaLightName) == raw,
                    "zone-to-main light resolution compares the same raw byte identity");
            non_ascii_rows += non_ascii(raw);
            verified_main_names.insert(raw);
            const auto saved = published->mAreaLightName;
            const auto display = resource::decode_cp932(raw);
            require(resource::decode_cp932(published->mAreaLightName) == display,
                    "host presentation explicitly decodes the original CP932 identity");
            require(saved == published->mAreaLightName && std::string_view(saved) == raw,
                    "presentation conversion does not replace the Game-owned name");
        }
        if (!first_zone_rows.empty()) {
            auto fallback = first_zone_rows.find(-1);
            if (fallback != first_zone_rows.end()) {
                require(std::string_view(LightFunction::getDefaultAreaLightName()) == fallback->second,
                        "Game default-name accessor returns raw zone-table bytes");
            }
        }
        ++zone_tables;
    }
    require(zone_tables > 1 && non_ascii_rows > 1, "actual fixture covers multiple zone tables and Japanese names");

    const auto zones = std::array{render::light::StageLightZone{0,"HeavensDoorGalaxy"},
                                 render::light::StageLightZone{5,"HeavensDoorMysteriousZone"}};
    lights.configure_stage_zones(zones);
    lights.load_stage(dvd,"HeavensDoorGalaxy");
    ZoneLightID root;
    ZoneLightID child; child._0 = 5; child.mLightID = 0;
    auto* root_info = lights.area_light_info(root);
    auto* child_info = lights.area_light_info(child);
    require(resource::decode_cp932(root_info->mAreaLightName) == "[共通]宇宙の星",
            "explicit host decoding displays the authored root light name");
    require(resource::decode_cp932(child_info->mAreaLightName) == "ロゼッタ出会い",
            "explicit host decoding displays the authored child light name");
    require(std::string_view(child_info->mAreaLightName) != "ロゼッタ出会い",
            "Game light identity is CP932 rather than host UTF-8");
    require(child_info->mPlayerLight.mInfo0.mColor.r == 90 && child_info->mPlayerLight.mColor.b == 115,
            "raw-name boundary preserves the authored selected light payload");
    lights.reset();
    require(LightFunction::getDefaultAreaLightName() == nullptr &&
                LightFunction::getAreaLightInfo(root) == nullptr,
            "reset unpublishes raw-name storage through Game accessors");
    std::cout << "[pass] main_rows=" << main.entry_count() << " main_non_ascii=" << main_non_ascii
              << " zone_tables=" << zone_tables << " zone_rows=" << zone_rows
              << " verified_main_names=" << verified_main_names.size()
              << " verified_non_ascii_rows=" << non_ascii_rows << " unmatched_rows=" << unmatched_rows
              << " raw Game identities; explicit UTF-8 presentation; reset\n";
}
}
int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the actual disc fixture");
        require(aurora_dvd_open(disc), "actual disc fixture opens");
        struct Close { ~Close() { aurora_dvd_close(); } } close;
        DVDInit();
        smgpc::runtime::DvdFileSystemService dvd("/");
        exercise(dvd);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
