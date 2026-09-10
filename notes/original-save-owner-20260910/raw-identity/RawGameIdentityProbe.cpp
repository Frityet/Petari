#include "compat/DemoSheetRuntime.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "scene/nameobj/ObjectNameTable.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
using Row = std::vector<std::string>;
using smgpc::resource::RarcArchive;
unsigned checks = 0;
void require(bool value, const char* why) {
    ++checks;
    if (!value) throw std::runtime_error(why);
}
void be16(Bytes& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = value >> 8;
    bytes[offset + 1] = value;
}
void be32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = value >> 24;
    bytes[offset + 1] = value >> 16;
    bytes[offset + 2] = value >> 8;
    bytes[offset + 3] = value;
}
Bytes bcsv(const std::vector<std::string>& fields, const std::vector<Row>& rows) {
    const auto data = 16 + fields.size() * 12;
    const auto strings = data + rows.size() * fields.size() * 4;
    Bytes bytes(strings, 0);
    be32(bytes, 0, rows.size()); be32(bytes, 4, fields.size());
    be32(bytes, 8, data); be32(bytes, 12, fields.size() * 4);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto offset = 16 + i * 12;
        be32(bytes, offset, smgpc::resource::jmap_hash(fields[i]));
        be32(bytes, offset + 4, 0xffffffff);
        be16(bytes, offset + 8, i * 4);
        bytes[offset + 11] = static_cast<std::uint8_t>(smgpc::resource::BcsvFieldType::StringOffset);
    }
    for (std::size_t i = 0; i < rows.size(); ++i) {
        require(rows[i].size() == fields.size(), "fixture has complete source columns");
        for (std::size_t j = 0; j < fields.size(); ++j) {
            be32(bytes, data + (i * fields.size() + j) * 4, bytes.size() - strings);
            bytes.insert(bytes.end(), rows[i][j].begin(), rows[i][j].end());
            bytes.push_back(0);
        }
    }
    return bytes;
}
RarcArchive archive(const std::vector<std::pair<std::string, Bytes>>& files) {
    const std::size_t entries = 0x50;
    const auto strings = entries + files.size() * 20;
    std::size_t string_size = 0, data_size = 0;
    for (const auto& [name, contents] : files) {
        string_size += name.size() + 1;
        data_size += contents.size();
    }
    const auto data = (strings + string_size + 31) & ~std::size_t(31);
    Bytes bytes(data + data_size, 0);
    be32(bytes, 0, 0x52415243); be32(bytes, 4, bytes.size());
    be32(bytes, 8, 0x20); be32(bytes, 12, data - 0x20); be32(bytes, 16, data_size);
    be32(bytes, 0x20, 1); be32(bytes, 0x24, 0x20);
    be32(bytes, 0x28, files.size()); be32(bytes, 0x2c, entries - 0x20);
    be32(bytes, 0x30, string_size); be32(bytes, 0x34, strings - 0x20);
    be16(bytes, 0x4a, files.size());
    std::size_t string_offset = 0, data_offset = 0;
    for (std::size_t i = 0; i < files.size(); ++i) {
        const auto& [name, contents] = files[i];
        const auto entry = entries + i * 20;
        be16(bytes, entry, i); be16(bytes, entry + 2, RarcArchive::hash_name(name));
        bytes[entry + 4] = 1;
        bytes[entry + 5] = string_offset >> 16;
        bytes[entry + 6] = string_offset >> 8;
        bytes[entry + 7] = string_offset;
        be32(bytes, entry + 8, data_offset); be32(bytes, entry + 12, contents.size());
        std::copy(name.begin(), name.end(), bytes.begin() + strings + string_offset);
        std::copy(contents.begin(), contents.end(), bytes.begin() + data + data_offset);
        string_offset += name.size() + 1;
        data_offset += contents.size();
    }
    return RarcArchive::from_bytes(std::move(bytes));
}
void verify() {
    using smgpc::resource::decode_cp932;
    using smgpc::resource::encode_cp932;
    using smgpc::compat::DemoSheetRuntime;
    using smgpc::compat::DemoSheetStartResult;
    const std::string first{"\x87\x54"}, second{"\xfa\x4a"};
    require(first != second && decode_cp932(first) == decode_cp932(second), "independent CP932 byte aliases share presentation only");
    const auto game_name = encode_cp932("チコ");
    const auto animation = encode_cp932("基本");
    const auto talk_part = encode_cp932("スピンゲット[会話1]");
    auto names = [&] {
        const auto source = archive({{"ObjNameTable.tbl", bcsv({"en_name", "jp_name"},
            {{"Alias1", first}, {"Alias2", second}, {"Alias1", second}, {"Tico", game_name}, {"Empty", ""}})}});
        return smgpc::scene::nameobj::ObjectNameTable(source);
    }();
    require(names.size() == 4, "first duplicate English key is retained");
    require(*names.lookup("Alias1") == first && *names.lookup("Alias2") == second,
            "Game actor names preserve distinct authored CP932 aliases after archive destruction");
    require(*names.lookup("Tico") == game_name && *names.lookup("Empty") == "" && names.lookup("Missing") == nullptr,
            "Japanese, empty, and missing object names retain their original contracts");
    const auto* retained = names.lookup("Tico")->c_str();
    require(retained == names.lookup("Tico")->c_str() && std::string(retained) == game_name,
            "Game constructor borrows stable table-owned bytes");
    require(decode_cp932(*names.lookup("Tico")) == "チコ", "object-name presentation is explicitly UTF-8");

    auto sheet = [&] {
        const auto source = archive({
            {"DemoRawTime.bcsv", bcsv({"PartName"}, {{first}, {second}})},
            {"DemoRawSubPart.bcsv", bcsv({"SubPartName", "MainPartName"}, {{talk_part, first}})},
            {"DemoRawPlayer.bcsv", bcsv({"PartName", "PosName", "BckName"}, {{first, second, animation}})},
            {"DemoRawCamera.bcsv", bcsv({"PartName", "CameraTargetName", "AnimCameraName"}, {{first, game_name, animation}})},
            {"DemoRawAction.bcsv", bcsv({"PartName", "CastName", "PosName", "AnimName"}, {{second, game_name, first, animation}})},
            {"DemoRawWipe.bcsv", bcsv({"PartName"}, {{first}})},
            {"DemoRawSound.bcsv", bcsv({"PartName", "Bgm", "SystemSe"}, {{second, animation, game_name}})},
        });
        return DemoSheetRuntime::load(source, "Raw");
    }();
    require(sheet.time_rows().size() == 2 && sheet.time_rows()[0].part_name == first && sheet.time_rows()[1].part_name == second,
            "time rows preserve raw alias order after archive destruction");
    require(sheet.start_at_part(decode_cp932(first)) == DemoSheetStartResult::PartNotFound,
            "UTF-8 aliases are not silently admitted to Game identity lookup");
    require(sheet.start_at_part(second) == DemoSheetStartResult::Started && sheet.current_part_index() == 1,
            "the second CP932 alias selects the second authored row");
    const char* part = sheet.current_part()->part_name.c_str();
    sheet.stop();
    require(sheet.start_at_part(first) == DemoSheetStartResult::Started && sheet.current_part_index() == 0,
            "the first CP932 alias remains independently selectable");
    require(std::string(part) == second && part == sheet.time_rows()[1].part_name.c_str(),
            "borrowed original part name survives stop and restart without temporary conversion");
    require(sheet.sub_part_rows()[0].sub_part_name == talk_part && sheet.sub_part_rows()[0].main_part_name == first,
            "subpart links use source bytes");
    require(sheet.player_rows()[0].position_name == second && sheet.player_rows()[0].bck_name == animation,
            "original player position and BCK requests retain Game bytes");
    require(sheet.camera_rows()[0].target_name == game_name && sheet.camera_rows()[0].animation_name == animation,
            "camera row names retain Game bytes");
    require(sheet.action_rows()[0].cast_name == game_name && sheet.action_rows()[0].position_name == first &&
                sheet.action_rows()[0].animation_name == animation, "original action target, position and animation names retain Game bytes");
    require(sheet.wipe_rows()[0].wipe_name == encode_cp932("フェードワイプ"), "missing wipe name has the original CP932 default");
    require(decode_cp932(sheet.wipe_rows()[0].wipe_name) == "フェードワイプ", "native wipe presentation is explicitly decoded");
    require(sheet.sound_rows()[0].bgm == animation && sheet.sound_rows()[0].system_sound == game_name,
            "sound row identifiers retain Game bytes");
    require(MR::isDemoPartTalk(talk_part.c_str()) && !MR::isDemoPartTalk(animation.c_str()) &&
                !MR::isDemoPartTalk("会話"), "actual demo talk predicate recognizes only the original byte substring");
}
} // namespace
int main() {
    try {
        verify();
        std::cout << "[ok] " << checks << " checks: raw Game actor/demo identities, CP932 aliases, borrowed lifetime and explicit presentation\n";
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
