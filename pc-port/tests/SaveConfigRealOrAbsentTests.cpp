#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/ConfigDataHolder.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template <typename Exception>
void require_throws(const std::function<void()>& operation, std::string_view message) {
    auto threw = false;
    try {
        operation();
    } catch (const Exception&) {
        threw = true;
    }
    require(threw, message);
}

[[nodiscard]] std::string read_file(const std::filesystem::path& path) {
    auto stream = std::ifstream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Could not open source evidence: " + path.string());
    }
    auto buffer = std::ostringstream{};
    buffer << stream.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::filesystem::path find_project_root() {
    for (auto path = std::filesystem::current_path(); !path.empty(); path = path.parent_path()) {
        if (std::filesystem::is_regular_file(path / "src/Game/System/ConfigDataHolder.cpp") &&
            std::filesystem::is_regular_file(path / "../src/Game/System/ConfigDataHolder.cpp")) {
            return path;
        }
        if (path == path.root_path()) {
            break;
        }
    }
    throw std::runtime_error("Could not locate the pc-port project root");
}

void test_decompiled_sources_are_byte_exact() {
    const auto project = find_project_root();
    constexpr auto sources = std::array{
        "BinaryDataChunkHolder.cpp", "BinaryDataContentAccessor.cpp", "ConfigDataHolder.cpp",
        "ConfigDataMii.cpp", "ConfigDataMisc.cpp", "SaveDataHandleSequence.cpp", "SysConfigFile.cpp",
        "UserFile.cpp",
    };
    for (const auto* name : sources) {
        require(read_file(project / "src/Game/System" / name) == read_file(project / "../src/Game/System" / name),
                std::string("pc-port Game source differs from the decomp: ") + name);
    }

    constexpr auto headers = std::array{
        "BinaryDataChunkHolder.hpp", "BinaryDataContentAccessor.hpp", "ConfigDataHolder.hpp",
        "ConfigDataMii.hpp", "ConfigDataMisc.hpp", "SaveDataHandleSequence.hpp", "SysConfigFile.hpp",
        "UserFile.hpp",
    };
    for (const auto* name : headers) {
        require(read_file(project / "src/Game/System" / name) == read_file(project / "../include/Game/System" / name),
                std::string("pc-port Game header differs from the decomp: ") + name);
    }
}

void test_config_binary_matches_dolphin_oracle() {
    constexpr auto expected = std::array<std::uint8_t, 60U>{
        0x01U, 0x03U, 0x00U, 0x00U, 0x43U, 0x4fU, 0x4eU, 0x46U, 0x00U, 0x24U, 0x32U, 0xdaU,
        0x00U, 0x00U, 0x00U, 0x0dU, 0x00U, 0x4dU, 0x49U, 0x49U, 0x20U, 0x00U, 0x28U, 0x36U,
        0xe9U, 0x00U, 0x00U, 0x00U, 0x16U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x01U, 0x4dU, 0x49U, 0x53U, 0x43U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U,
        0x00U, 0x00U, 0x15U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    };

    auto config = ConfigDataHolder{};
    auto bytes = std::array<std::uint8_t, 96U>{};
    const auto size = config.makeFileBinary(bytes.data(), bytes.size());
    require(size == static_cast<s32>(expected.size()) &&
                std::equal(expected.begin(), expected.end(), bytes.begin()),
            "default config binary must match the bytes captured from Dolphin GameData.bin");
    require(std::all_of(bytes.begin() + size, bytes.end(), [](auto byte) { return byte == 0U; }),
            "retail config padding must remain zero");

    auto loaded = ConfigDataHolder{};
    require(loaded.loadFromFileBinary("config1", bytes.data(), bytes.size()),
            "the exact config holder must accept the Dolphin-compatible binary");
    auto corrupt = bytes;
    corrupt[11U] ^= 1U;
    require(!loaded.loadFromFileBinary("config1", corrupt.data(), corrupt.size()),
            "a config chunk with the wrong retail hash must be rejected");
}

void test_sysconfig_uses_proven_retail_chunk() {
    auto config = SysConfigFile{};
    config.setTimeSent(static_cast<OSTime>(0x0102030405060708ULL));
    config.setSentBytes(0xaabbccddU);
    auto bytes = std::array<std::uint8_t, 0x3000U>{};
    config.makeDataBinary(bytes.data(), bytes.size());

    require(BinaryDataChunkHolder::calcBinarySize(bytes.data()) == 52U,
            "SYSC must use the retail binary-chunk file and chunk lengths");
    require(bytes[0U] == 1U && bytes[1U] == 1U &&
                std::string_view(reinterpret_cast<const char*>(bytes.data() + 4U), 4U) == "SYSC" &&
                bytes[11U] == 1U && bytes[15U] == 48U,
            "SYSC must expose the retail version, signature, hash, and chunk size");
    require(bytes[16U] == 0U && bytes[17U] == 3U && bytes[18U] == 0U && bytes[19U] == 20U &&
                bytes[20U] == 0xa5U && bytes[21U] == 0x61U && bytes[24U] == 0x0fU && bytes[25U] == 0x92U &&
                bytes[28U] == 0x49U && bytes[29U] == 0xc6U,
            "SYSC must retain the decompiled attribute table hashes and sizes");
    constexpr auto sent_time = std::array<std::uint8_t, 8U>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    require(std::equal(sent_time.begin(), sent_time.end(), bytes.begin() + 40U) &&
                bytes[48U] == 0xaaU && bytes[49U] == 0xbbU && bytes[50U] == 0xccU && bytes[51U] == 0xddU,
            "SYSC scalar data must use the retail big-endian byte order");

    auto loaded = SysConfigFile{};
    loaded.loadFromDataBinary(bytes.data(), bytes.size());
    require(loaded.getTimeAnnounced() == 0 && loaded.getTimeSent() == static_cast<OSTime>(0x0102030405060708ULL) &&
                loaded.getSentBytes() == 0xaabbccddU,
            "retail SYSC data must round-trip through the host compatibility boundary");
    require_throws<std::invalid_argument>([&] { loaded.loadFromDataBinary(bytes.data(), 51U); },
                                          "truncated SYSC data must fail explicitly");
}

void test_save_service_is_real_or_absent() {
    auto service = smgpc::runtime::SaveDataService{};
    require(!service.read_file("GameData.bin").has_value() && !service.has_valid_game_data_container(),
            "an unconfigured save service must not synthesize GameData.bin");
    const auto fake = std::array<std::uint8_t, 1U>{0U};
    require_throws<std::logic_error>([&] { service.write_file("GameData.bin", fake); },
                                     "save persistence must be unavailable without an explicit host directory");

    const auto project = find_project_root();
    const auto oracle = project / "notes/dolphin-oracle-20260806T201750Z/seed-dolphin-user/Wii/title/00010000/524d474b/data";
    require(std::filesystem::is_regular_file(oracle / "GameData.bin"),
            "the checked Dolphin oracle GameData.bin must be present");
    service.set_host_directory(oracle);
    require(service.has_valid_game_data_container(),
            "an actual Dolphin retail GameData.bin must be recognized");
    const auto host_view = service.read_nand_file("GameData.bin");
    require(host_view.has_value() && host_view->size() == 0xbe00U,
            "the Aurora NAND boundary must expose the actual persisted container");
    auto version = std::uint32_t{};
    std::memcpy(&version, host_view->data() + 4U, sizeof(version));
    require(version == 2U,
            "only the outer PPC-struct ABI is translated for the host; the real payload remains authoritative");
}
}  // namespace

int main() {
    test_decompiled_sources_are_byte_exact();
    test_config_binary_matches_dolphin_oracle();
    test_sysconfig_uses_proven_retail_chunk();
    test_save_service_is_real_or_absent();
    std::cout << "Save/config real-or-absent tests passed: 4/4\n";
    return 0;
}
