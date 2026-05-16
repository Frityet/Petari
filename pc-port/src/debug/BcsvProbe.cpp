#include "Game/compat/BcsvTable.hpp"
#include "Game/compat/RarcArchive.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

    [[nodiscard]] std::unordered_map< std::uint32_t, std::string_view > known_field_hashes() {
        constexpr std::array< std::string_view, 58U > names{
            "version",
            "id",
            "camtype",
            "woffset.X",
            "woffset.Y",
            "woffset.Z",
            "loffset",
            "loffsetv",
            "roll",
            "fovy",
            "camint",
            "upper",
            "lower",
            "gndint",
            "uplay",
            "lplay",
            "pushdelay",
            "pushdelaylow",
            "udown",
            "vpanuse",
            "vpanaxis.X",
            "vpanaxis.Y",
            "vpanaxis.Z",
            "flag.noreset",
            "flag.nofovy",
            "flag.lofserpoff",
            "flag.antibluroff",
            "flag.collisionoff",
            "flag.subjectiveoff",
            "dist",
            "axis.X",
            "axis.Y",
            "axis.Z",
            "wpoint.X",
            "wpoint.Y",
            "wpoint.Z",
            "up.X",
            "up.Y",
            "up.Z",
            "angleA",
            "angleB",
            "num1",
            "num2",
            "string",
            "gflag.thru",
            "gflag.enableEndErpFrame",
            "gflag.camendint",
            "eflag.enableErpFrame",
            "eflag.enableEndErpFrame",
            "camendint",
            "evfrm",
            "evpriority",
            "l_id",
            "Obj_arg0",
            "Obj_arg1",
            "Obj_arg2",
            "Obj_arg3",
            "CameraSetId",
        };

        auto hashes = std::unordered_map< std::uint32_t, std::string_view >{};
        for (const auto name : names) {
            hashes.emplace(smgpc::game::jmap_hash(name), name);
        }
        return hashes;
    }

    [[nodiscard]] std::string field_name(const std::unordered_map< std::uint32_t, std::string_view >& hashes, std::uint32_t hash) {
        const auto it = hashes.find(hash);
        if (it != hashes.end()) {
            return std::string(it->second);
        }

        auto stream = std::ostringstream{};
        stream << "hash_0x" << std::hex << hash;
        return stream.str();
    }

    [[nodiscard]] std::filesystem::path resolve_archive_path(std::string_view archive_name) {
        const auto requested = std::filesystem::path(archive_name);
        if (requested.is_absolute() || requested.has_parent_path()) {
            return requested;
        }

        return std::filesystem::current_path() / archive_name;
    }

    [[nodiscard]] bool is_string_field(smgpc::game::BcsvFieldType type) {
        return type == smgpc::game::BcsvFieldType::InlineString || type == smgpc::game::BcsvFieldType::StringOffset;
    }

}  // namespace

int main(int argc, char** argv) try {
    if (argc < 3) {
        std::cerr << "usage: smg-pc-bcsv-probe <archive.arc> <entry-path>\n";
        return 1;
    }

    const auto archive = smgpc::game::RarcArchive::from_file(resolve_archive_path(argv[1]));
    const auto table = smgpc::game::BcsvTable::from_bytes(archive.file_data(argv[2]));
    const auto hashes = known_field_hashes();

    std::cout << "entries," << table.entry_count() << ",fields," << table.fields().size() << ",entry_size," << table.entry_size() << '\n';
    for (auto entry = 0U; entry < table.entry_count(); ++entry) {
        std::cout << "entry," << entry << '\n';
        for (std::size_t field_index = 0U; field_index < table.fields().size(); ++field_index) {
            const auto& field = table.fields()[field_index];
            const auto value = table.value_string(entry, field_index);
            std::cout << "  " << field_name(hashes, field.hash) << '=';
            if (is_string_field(field.type)) {
                std::cout << '"' << value << '"';
            } else {
                std::cout << value;
            }
            std::cout << " (type " << smgpc::game::bcsv_field_type_name(field.type) << ", offs " << field.offset << ")\n";
        }
    }

    return 0;
} catch (const std::exception& e) {
    std::cerr << "BCSV probe failed: " << e.what() << '\n';
    return 1;
}
