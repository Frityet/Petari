#include "Game/Effect/AutoEffectGroup.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 2);
    auto arc = smgpc::resource::RarcArchive::from_file(argv[1]);
    const auto bytes = arc.file_data_by_basename("AutoEffectList.bcsv");
    const auto table = smgpc::resource::BcsvTable::from_bytes(bytes);
    auto map = JMapInfo::from_bcsv(bytes);
    std::map<std::string, std::vector<int>> groups;
    std::size_t colors = 0;
    std::array<std::size_t, 9> draw_counts{};
    constexpr const char* orders[]{"3D", "PAUSE_IGNORE", "INDIRECT", "AFTER_INDIRECT", "BLOOM_EFFECT", "AFTER_IMAGE_EFFECT", "2D", "2D_PAUSE_IGNORE", "FOR_2D_MODEL"};
    for (int row = 0; row < map.getNumEntries(); ++row) {
        const char* name = nullptr;
        assert(map.getValue(row, "GroupName", &name));
        groups[name].push_back(row);
    }
    // Every actual record goes through the original allocating group/add/init graph.
    for (const auto& [name, rows] : groups) {
        AutoEffectGroup group(name.c_str(), static_cast<int>(rows.size()));
        for (const int row : rows) {
            group.add(JMapInfoIter(&map, row));
            const AutoEffectInfo& info = *group.mInfos[group.mInfos.size() - 1];
            auto str = [&](const char* field) { return table.get_string(row, field).value(); };
            unsigned expected_flags = str("ContinueAnimEnd") == "on" ? 64 : 0;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const char letter = "TRS"[axis];
                if (str("Follow").find(letter) != std::string::npos) expected_flags |= 1 << axis;
                if (str("Affect").find(letter) != std::string::npos) expected_flags |= 8 << axis;
            }
            assert(info.mFlag == expected_flags);
            const std::array<std::pair<const char*, float>, 6> floats{{
                {"OffsetX", info.mOffsetX}, {"OffsetY", info.mOffsetY}, {"OffsetZ", info.mOffsetZ},
                {"ScaleValue", info.mScaleValue}, {"RateValue", info.mRateValue}, {"LightAffectValue", info.mLightAffectValue}}};
            for (const auto& [field, actual] : floats) assert(actual == table.get_float(row, field).value());
            assert(info.mStartFrame == table.get_s32(row, "StartFrame").value());
            assert(info.mEndFrame == table.get_s32(row, "EndFrame").value());
            auto check_color = [&](const char* field, const Color8& actual, bool valid) {
                const auto value = str(field);
                assert(valid == !value.empty());
                if (valid) {
                    const auto expected = static_cast<std::uint32_t>(std::stoul(value.substr(1), nullptr, 16)) << 8;
                    assert(actual.r == ((expected >> 24) & 255));
                    assert(actual.g == ((expected >> 16) & 255));
                    assert(actual.b == ((expected >> 8) & 255));
                    assert(actual.a == 0 && static_cast<u32>(actual) == expected);
                    ++colors;
                }
            };
            check_color("PrmColor", info.mPrmColor, info.mIsValidPrmColor);
            check_color("EnvColor", info.mEnvColor, info.mIsValidEnvColor);
            int order = 0;
            for (int i = 0; i < 9; ++i) if (str("DrawOrder") == orders[i]) order = i;
            assert(info.mDrawOrder == order);
            ++draw_counts[order];
            const std::array<std::pair<const char*, const char*>, 6> strings{{
                {"GroupName", info.mGroupName}, {"AnimName", info.mAnimName}, {"UniqueName", info.mUniqueName},
                {"EffectName", info.mEffectName}, {"ParentName", info.mParentName}, {"JointName", info.mJointName}}};
            for (const auto& [field, actual] : strings) {
                const char* expected = nullptr;
                assert(map.getValue(row, field, &expected));
                assert(*expected ? actual && std::string(actual) == expected : actual == nullptr);
            }
            assert(info.getName() == (info.mUniqueName ? info.mUniqueName : info.mEffectName));
        }
        assert(group.mInfos.size() == rows.size());
        for (int i = 0; i < group.mInfos.size(); ++i) delete group.mInfos[i];
    }
    // Integer/channel/GXColor round trips exercise the general color boundary.
    std::size_t channel_checks = 0;
    for (unsigned channel = 0; channel < 4; ++channel) {
        for (unsigned value = 0; value < 256; ++value) {
            const std::uint32_t bits = (0x19a5d37b & ~(255u << (channel * 8))) | (value << (channel * 8));
            const Color8 color(bits);
            assert(color.r == (bits >> 24) && color.g == ((bits >> 16) & 255) && color.b == ((bits >> 8) & 255) && color.a == (bits & 255));
            assert(static_cast<u32>(Color8(static_cast<GXColor>(color))) == bits);
            ++channel_checks;
        }
    }
    std::cout << "records=" << map.getNumEntries() << " groups=" << groups.size() << " authored_colors=" << colors << " color_channel_roundtrips=" << channel_checks << '\n';
    for (int i = 0; i < 9; ++i) std::cout << orders[i] << '=' << draw_counts[i] << ' ';
    std::cout << '\n';
}
