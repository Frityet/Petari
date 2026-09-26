#include "SourceMirrorEncoding.hpp"
#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/ConfigDataHolder.hpp"
#include "Game/System/ConfigDataMisc.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "common/BinaryChunkFile.hpp"
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
#include <aurora/endian.hpp>

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
            std::filesystem::is_regular_file(path / "decomp/src/Game/System/ConfigDataHolder.cpp")) {
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
        "ConfigDataHolder.cpp", "ConfigDataMii.cpp", "BinaryDataChunkHolder.cpp",
        "UserFile.cpp",
    };
    for (const auto* name : sources) {
        require(smgpc::test::source_matches_with_cp932(read_file(project / "decomp/src/Game/System" / name), read_file(project / "src/Game/System" / name)),
                std::string("pc-port Game source differs from the decomp: ") + name);
    }

    constexpr auto headers = std::array{
        "ConfigDataHolder.hpp", "ConfigDataMii.hpp", "SaveDataHandleSequence.hpp",
        "UserFile.hpp",
    };
    for (const auto* name : headers) {
        require(smgpc::test::source_matches_with_cp932(read_file(project / "decomp/include/Game/System" / name), read_file(project / "src/Game/System" / name)),
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
    require(!smgpc::common::has_bounded_binary_chunks(std::span(bytes).first(51)),
            "the host boundary rejects a truncated SYSC chunk extent");
    loaded.loadFromDataBinary(bytes.data(), 51U);
    require(loaded.getTimeSent() == static_cast<OSTime>(0x0102030405060708ULL) && loaded.getSentBytes() == 0xaabbccddU,
            "the original SysConfigFile ignores a failed load and retains existing data");

    auto reject = [&](const auto& malformed) {
        loaded.setTimeSent(37);
        loaded.setSentBytes(41);
        loaded.loadFromDataBinary(malformed.data(), malformed.size());
        require(loaded.getTimeAnnounced() == 0 && loaded.getTimeSent() == 37 && loaded.getSentBytes() == 41,
                "SYSC validates its entire schema before changing any existing values");
    };
    auto corrupt16 = [&](std::size_t offset, u16 value) {
        auto malformed = bytes;
        aurora::endian::write_u16(malformed.data() + offset, value);
        reject(malformed);
    };
    corrupt16(16, 0xffff); // Descriptor count exceeds the chunk.
    corrupt16(18, 19); // Record cannot contain all required values.
    corrupt16(20, 0); // Required announced-time descriptor is missing.
    corrupt16(22, 13); // Eight-byte field extends past the record.
    corrupt16(26, 0); // Both times refer to the same bytes.

    // A larger schema can reorder fields and retain unknown descriptors.
    std::array<u8, 61> extended{};
    std::copy_n(bytes.begin(), 16, extended.begin());
    aurora::endian::write_big(extended.data() + 12, u32{57});
    aurora::endian::write_u16(extended.data() + 16, 4);
    aurora::endian::write_u16(extended.data() + 18, 25);
    constexpr std::array<u8, 16> attributes{
        0x49, 0xc6, 0, 0, 0x0f, 0x92, 0, 5, 0xa5, 0x61, 0, 13, 0x12, 0x34, 0, 25,
    };
    std::copy(attributes.begin(), attributes.end(), extended.begin() + 20);
    aurora::endian::write_big(extended.data() + 36, u32{0x89abcdef});
    aurora::endian::write_big(extended.data() + 41, u64{0x1122334455667788});
    aurora::endian::write_big(extended.data() + 49, u64{0xfedcba9876543210});
    loaded.loadFromDataBinary(extended.data(), extended.size());
    require(loaded.getTimeSent() == static_cast<OSTime>(0x1122334455667788) &&
                loaded.getTimeAnnounced() == static_cast<OSTime>(0xfedcba9876543210ULL) && loaded.getSentBytes() == 0x89abcdef,
            "SYSC accepts reordered, unaligned, signed-time fields with unknown schema extensions");
}

void test_misc_legacy_and_signed_stream_bounds() {
    ConfigDataMisc misc;
    constexpr std::array<u8, 9> golden{7, 1, 2, 3, 4, 5, 6, 7, 8};
    require(misc.deserialize(golden.data(), golden.size()) == 0 && misc.getLastModified() == 0x0102030405060708LL,
            "MISC decodes the fixed big-endian timestamp");
    std::array<u8, 9> serialized{};
    require(misc.serialize(serialized.data(), serialized.size()) == serialized.size() && serialized == golden,
            "MISC serializes the exact flag and timestamp bytes");
    require(misc.deserialize(golden.data(), 1) == 0 && misc.isOnCompleteEndingMario() &&
                misc.isOnCompleteEndingLuigi() && misc.getLastModified() == 0,
            "Legacy one-byte MISC retains flags and defaults the absent timestamp");
    for (u32 size = 2; size < golden.size(); ++size)
        require(misc.deserialize(golden.data(), size) != 0, "MISC rejects partial timestamps");
    require(misc.deserialize(golden.data(), 0xffffffffU) != 0, "Oversized MISC input fails without reading an uninitialized flag");
    require_throws<std::length_error>([&] { misc.serialize(serialized.data(), 0xffffffffU); },
                                      "Oversized MISC output fails instead of producing an empty chunk");
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

    const auto original = *service.read_file("GameData.bin");
    const auto file_size = aurora::endian::read_u32(original.data() + 12);
    const auto first_member = aurora::endian::read_u32(original.data() + 28);
    auto publish_in_memory = [&](std::vector<u8> candidate) {
        u16 sum = 0, inverse = 0;
        for (std::size_t offset = 4; offset + 1 < file_size; offset += 2) {
            const auto word = aurora::endian::read_u16(candidate.data() + offset);
            sum += word;
            inverse += static_cast<u16>(~word);
        }
        aurora::endian::write_big(candidate.data(), (u32(sum) << 16) | inverse);
        service.nand().write_file("GameData.bin", candidate);
    };
    auto malformed = original;
    aurora::endian::write_big(malformed.data() + first_member + 12, u32{0xffffffff});
    publish_in_memory(malformed);
    require_throws<std::runtime_error>([&] { static_cast<void>(service.read_nand_file("GameData.bin")); },
                                      "NAND rejects an out-of-bounds chunk even with a valid outer checksum");
    auto changed_hash = original;
    changed_hash[first_member + 8] ^= 1;
    publish_in_memory(changed_hash);
    require(service.read_nand_file("GameData.bin").has_value(),
            "NAND leaves chunk hash handling to the original loader");
    malformed = original;
    aurora::endian::write_big(malformed.data() + 12, u32{0xfffffff0});
    service.nand().write_file("GameData.bin", malformed);
    require_throws<std::runtime_error>([&] { static_cast<void>(service.read_nand_file("GameData.bin")); },
                                      "NAND rejects a wrapped outer size before reading its checksum span");
    service.nand().write_file("GameData.bin", original);
}

void test_original_chunk_error_continuation() {
    struct Chunk final : BinaryDataChunkBase {
        u32 signature, hash;
        u8 value;
        s32 status = 0;
        std::vector<u32>* visits = nullptr;
        Chunk(u32 signature, u32 hash, u8 value) : signature(signature), hash(hash), value(value) {}
        u32 getSignature() const override { return signature; }
        u32 makeHeaderHashCode() const override { return hash; }
        s32 serialize(u8* bytes, u32 size) const override {
            require(size >= 1, "chunk fixture output capacity");
            bytes[0] = value;
            return 1;
        }
        s32 deserialize(const u8* bytes, u32 size) override {
            require(size == 1, "the original reader passes the precise payload size");
            value = bytes[0];
            visits->push_back(signature);
            return status;
        }
        void initializeData() override { value = 0; }
    };
    Chunk first(1, 10, 0x11), unknown(99, 99, 0x22), second(2, 20, 0x33), last(3, 30, 0x44);
    BinaryDataChunkHolder source(32, 4);
    for (auto* chunk : {&first, &unknown, &second, &last}) source.addChunk(chunk);
    std::array<u8, 64> bytes{};
    const auto size = source.makeFileBinary(bytes.data(), bytes.size());
    require(size == 56 && BinaryDataChunkHolder::calcBinarySize(bytes.data()) == size,
            "the original serializer packs unaligned thirteen-byte chunks");
    for (std::size_t length = 0; length < size; ++length)
        require(!smgpc::common::has_bounded_binary_chunks(std::span(bytes).first(length)),
                "the host rejects every truncated file extent before original unsized reads");
    require(smgpc::common::has_bounded_binary_chunks(bytes), "valid file padding is allowed");
    auto malformed = bytes;
    aurora::endian::write_big(malformed.data() + 12, u32{0xffffffff});
    require(!smgpc::common::has_bounded_binary_chunks(malformed), "overflowing chunk lengths are rejected");

    Chunk a(1, 11, 0), b(2, 20, 0), c(3, 30, 0);
    std::vector<u32> visits;
    BinaryDataChunkHolder destination(32, 3);
    for (auto* chunk : {&a, &b, &c}) {
        chunk->visits = &visits;
        destination.addChunk(chunk);
    }
    require(!destination.loadFromFileBinary(bytes.data(), size) && a.value == 0x11 && b.value == 0x33 && c.value == 0x44 &&
                visits == std::vector<u32>{1, 2, 3},
            "hash mismatch still deserializes matching chunks and skips unknown signatures");
    visits.clear();
    a.hash = 10;
    b.status = 2;
    c.value = 0;
    require(!destination.loadFromFileBinary(bytes.data(), size) && c.value == 0x44 && visits == std::vector<u32>{1, 2, 3},
            "deserialize failure is reported after processing later chunks");
    b.status = 1;
    require(destination.loadFromFileBinary(bytes.data(), size), "original status one is a successful load");
}
}  // namespace

int main() {
    test_decompiled_sources_are_byte_exact();
    test_config_binary_matches_dolphin_oracle();
    test_sysconfig_uses_proven_retail_chunk();
    test_misc_legacy_and_signed_stream_bounds();
    test_save_service_is_real_or_absent();
    test_original_chunk_error_continuation();
    std::cout << "Save/config real-or-absent tests passed: 6/6\n";
    return 0;
}
