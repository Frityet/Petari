#include "compat/SaveChunkEncoding.hpp"
#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GameEventValueChecker.hpp"
#include "Game/Util/HashUtil.hpp"
#include "resource/TextEncoding.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<u8>;
using namespace smgpc::compat;
constexpr u32 PLAY = 0x504c4159, FLG1 = 0x464c4731, PCE1 = 0x50434531;
constexpr u32 SPN1 = 0x53504e31, VLE1 = 0x564c4531, GALA = 0x47414c41;
unsigned checks = 0;
void require(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
void host16(Bytes& bytes, u16 value) {
    const auto offset = bytes.size();
    bytes.resize(offset + 2);
    std::memcpy(bytes.data() + offset, &value, 2);
}

// A byte source/sink for format-only vectors. This deliberately has no Game
// state and does not substitute for tests of original serializers below.
struct ByteChunk final : BinaryDataChunkBase {
    u32 signature;
    Bytes source;
    Bytes loaded;
    unsigned reads = 0;
    explicit ByteChunk(u32 type, Bytes bytes) : signature(type), source(std::move(bytes)) {}
    u32 getSignature() const override { return signature; }
    u32 makeHeaderHashCode() const override { return 0; }
    s32 serialize(u8* bytes, u32 capacity) const override {
        if (capacity < source.size()) return -1;
        std::memcpy(bytes, source.data(), source.size());
        return source.size();
    }
    s32 deserialize(const u8* bytes, u32 size) override {
        ++reads;
        loaded.assign(bytes, bytes + size);
        return 0;
    }
    void initializeData() override {}
};
void round_trip(u32 signature, const Bytes& native, const Bytes& wire) {
    ByteChunk chunk(signature, native);
    Bytes encoded(native.size() + 2, 0xcd);
    require(serialize_save_chunk(chunk, encoded.data() + 1, native.size()) == native.size(), "format encoded size");
    require(encoded.front() == 0xcd && encoded.back() == 0xcd, "format output canaries");
    require(std::equal(wire.begin(), wire.end(), encoded.begin() + 1), "format exact independent Wii bytes");
    require(validate_save_chunk_payload(signature, wire.data(), wire.size()), "valid format accepted");
    require(deserialize_save_chunk(chunk, wire.data(), wire.size()) == 0 && chunk.loaded == native,
            "Wii bytes become exact host-native original payload");
}

void play() {
    GameDataPlayerStatus status;
    status.mStoryProgress = 0x2a;
    status.mStockedStarPiece = 0x01020304;
    status.mPlayerLeft = 0x1234;
    constexpr std::array<u8, 7> expected{0x2a, 1, 2, 3, 4, 0x12, 0x34};
    for (u32 size = 0; size <= 9; ++size) {
        std::array<u8, 12> output;
        output.fill(0xcd);
        const auto copied = serialize_save_chunk(status, output.data() + 1, size);
        require(copied == std::min<u32>(size, expected.size()), "original PLAY prefix size");
        require(std::equal(expected.begin(), expected.begin() + copied, output.begin() + 1), "original PLAY Wii prefix");
        require(output.front() == 0xcd && output[copied + 1] == 0xcd, "original PLAY prefix canaries");
    }
    require(serialize_save_chunk(status, nullptr, 7) == 0, "null PLAY output");
    for (u32 size = 0; size <= expected.size(); ++size) {
        require(deserialize_save_chunk(status, expected.data(), size) == 0, "original PLAY legacy load status");
        u32 stock = 0;
        for (u32 i = 1; i < std::min<u32>(size, 5); ++i) stock |= u32(expected[i]) << ((4 - i) * 8);
        require(status.mStoryProgress == (size ? 0x2a : 0) && status.mStockedStarPiece == stock &&
                status.getPlayerLeft() == 4 && status.isPlayerLeftSupply() == (size >= 6),
                "original PLAY story/stock/supply and playable-life reset");
    }
    require(deserialize_save_chunk(status, nullptr, 7) == 0 && status.mStockedStarPiece == 0 &&
            status.getPlayerLeft() == 4 && !status.isPlayerLeftSupply(), "null PLAY defaults");
}

void flat_formats() {
    Bytes native;
    host16(native, 0xb5ba);
    host16(native, 0x1234);
    round_trip(FLG1, native, {0xb5, 0xba, 0x12, 0x34});
    native.clear();
    Bytes wire;
    for (u16 i = 0; i < 16; ++i) {
        host16(native, 0x1200 + i);
        wire.push_back(0x12);
        wire.push_back(i);
    }
    round_trip(PCE1, native, wire);
    require(!validate_save_chunk_payload(PCE1, wire.data(), 31), "short donation array rejected");
    require(!validate_save_chunk_payload(FLG1, wire.data(), 3), "partial flag rejected");
    require(!validate_save_chunk_payload(VLE1, wire.data(), 2), "partial event-value record rejected");
    round_trip(0x434f4e46, {0x12, 0x34, 0x56}, {0x12, 0x34, 0x56});
}

void paths() {
    Bytes native{1};
    host16(native, 0x1234); host16(native, 13);
    native.insert(native.end(), {1, 0});
    host16(native, 7);
    native.insert(native.end(), {0xc2, 0x85, 0x80, 0x46, 0xff});
    const Bytes wire{1, 0x12, 0x34, 0, 13, 1, 0, 0, 7, 0xc2, 0x85, 0x80, 0x46, 0xff};
    round_trip(SPN1, native, wire);
    for (std::size_t size = 0; size < wire.size(); ++size) {
        require(!validate_save_chunk_payload(SPN1, wire.data(), size), "each truncated path extent rejected");
    }
    for (const auto change : {std::array<u8, 2>{0, 2}, {4, 5}, {5, 2}, {8, 30}, {13, 0x85}, {9, 0x05}}) {
        auto bad = wire;
        bad[change[0]] = change[1];
        ByteChunk chunk(SPN1, native);
        require(deserialize_save_chunk(chunk, bad.data(), bad.size()) == -1 && chunk.reads == 0,
                "malformed path count/extent/tags cannot enter original unchecked reads");
    }
    round_trip(SPN1, {0}, {0});
}

void galaxies() {
    Bytes native;
    host16(native, 1); host16(native, 4); host16(native, 20);
    for (auto field : {std::array<u16, 2>{0x8208, 0}, {0x2196, 2}, {0xd423, 3}, {0x817e, 4}}) {
        host16(native, field[0]); host16(native, field[1]);
    }
    host16(native, 0x4567);
    native.insert(native.end(), {0x85, 0x12});
    for (u16 coin : {1, 0x1234, 0x0102, 0xabcd, 999, 0, 0xffff, 13}) host16(native, coin);
    const Bytes wire{0,1, 0,4, 0,20, 0x82,8,0,0, 0x21,0x96,0,2, 0xd4,0x23,0,3, 0x81,0x7e,0,4,
                     0x45,0x67,0x85,0x12, 0,1, 0x12,0x34, 1,2, 0xab,0xcd, 3,0xe7, 0,0, 0xff,0xff, 0,13};
    round_trip(GALA, native, wire);
    for (std::size_t size = 0; size < wire.size(); ++size) {
        require(!validate_save_chunk_payload(GALA, wire.data(), size), "each truncated galaxy extent rejected");
    }
    for (const auto change : {std::array<u8, 2>{1,2}, {3,255}, {5,0}, {6,0}, {21,5}, {13,1}}) {
        auto bad = wire;
        bad[change[0]] = change[1];
        ByteChunk chunk(GALA, native);
        require(deserialize_save_chunk(chunk, bad.data(), bad.size()) == -1 && chunk.reads == 0,
                "malformed galaxy dimensions/attribute widths/overlap rejected before virtual load");
    }
    // A version with only the required ID is supported by the original reader;
    // absent optional attributes become defaults there, not in this codec.
    Bytes minimal;
    for (u16 value : {1,1,2,0x8208,0,0x4567}) host16(minimal, value);
    round_trip(GALA, minimal, {0,1,0,1,0,2,0x82,8,0,0,0x45,0x67});

    // Descriptor order and record offsets are authored by the content header.
    // Unknown fields and later duplicate keys remain opaque, as the original
    // accessor resolves only the first matching key.
    Bytes extended;
    for (u16 value : {2,3,6,0xffff,2,0x8208,0,0x8208,4}) host16(extended, value);
    host16(extended, 0x1234);
    extended.insert(extended.end(), {0xaa,0xbb,0xcc,0xdd});
    host16(extended, 0x6789);
    extended.insert(extended.end(), {0x45,0x67,0x89,0xef});
    round_trip(GALA, extended,
               {0,2,0,3,0,6,0xff,0xff,0,2,0x82,8,0,0,0x82,8,0,4,
                0x12,0x34,0xaa,0xbb,0xcc,0xdd,0x67,0x89,0x45,0x67,0x89,0xef});

    Bytes reordered;
    for (u16 value : {1,4,20,0x817e,0,0xd423,19,0x8208,16,0x2196,18}) host16(reordered, value);
    for (u16 coin : {1,0x1234,0x0102,0xabcd,999,0,0xffff,13}) host16(reordered, coin);
    host16(reordered, 0x4567);
    reordered.insert(reordered.end(), {0x85,0x12});
    round_trip(GALA, reordered,
               {0,1,0,4,0,20,0x81,0x7e,0,0,0xd4,0x23,0,19,0x82,8,0,16,0x21,0x96,0,18,
                0,1,0x12,0x34,1,2,0xab,0xcd,3,0xe7,0,0,0xff,0xff,0,13,0x45,0x67,0x85,0x12});
}

void hashes_and_actual_values() {
    // This fixture remains a host UTF-8 translation unit. Original Game
    // sources are compiled through the real post-preprocessing CP932 wrapper.
    const auto book_name = smgpc::resource::encode_cp932("絵本既読章");
    const auto teresa_name = smgpc::resource::encode_cp932("テレサマリオ初変身");
    constexpr std::string_view book_bytes = "\212\107\226\173\212\371\223\307\217\315";
    constexpr std::string_view teresa_bytes = "\203\145\203\214\203\124\203\175\203\212\203\111\217\211\225\317\220\147";
    require(book_name == book_bytes && teresa_name == teresa_bytes,
            "host-to-Game conversion produces the literal original Japanese byte identities");
    require(MR::getHashCode("絵本既読章") == 0xba504313,
            "the production hash is a raw byte recurrence with no implicit UTF-8 conversion");
    require(MR::getHashCode(book_name.c_str()) == 0x1097f8c3,
            "the same raw hash produces the original CP932 value ID from Game bytes");
    int teresa = -1, cocoon = -1;
    for (int i = 0; i < GameEventFlagTable::getTableSize(); ++i) {
        const std::string_view name = GameEventFlagTable::getFlag(i)->mName;
        if (name == teresa_bytes) teresa = i;
        if (name == "CocoonExGalaxy") cocoon = i;
    }
    require(teresa >= 0 && cocoon >= 0, "the actual compiled flag table contains original CP932 literal bytes");
    const auto teresa_id = MR::getHashCode(teresa_name.c_str()) & 0x7fff;
    const auto cocoon_id = MR::getHashCode("CocoonExGalaxy") & 0x7fff;
    require(teresa_id == 0x278e && cocoon_id == 0x389a,
            "the formerly colliding flags retain their distinct original saved IDs");
    require(GameEventFlagTable::getIndexFromHashCode(teresa_id) == teresa &&
                GameEventFlagTable::getIndexFromHashCode(cocoon_id) == cocoon,
            "original linear lookup resolves both actual CP932 IDs without scope state or truncated-hash remapping");

    struct FlagTable {
        GameEventFlagTableInstance table;
        ~FlagTable() { delete[] table.mSortTable; }
    } flags;
    require(flags.table.findFlag(teresa_name.c_str()) == GameEventFlagTable::getFlag(teresa) &&
                flags.table.findFlag("CocoonExGalaxy") == GameEventFlagTable::getFlag(cocoon),
            "the actual original sorted gameplay lookup resolves both formerly colliding flags");
    std::set<u16> hashes;
    for (int i = 0; i < GameEventFlagTable::getTableSize(); ++i) {
        const auto* flag = GameEventFlagTable::getFlag(i);
        require(hashes.insert(static_cast<u16>(MR::getHashCode(flag->mName))).second,
                "all actual compiled original flag identities have distinct sorted keys");
        require(flags.table.findFlag(flag->mName) == flag,
                "each actual original flag resolves through its own sorted table instance");
    }

    struct Values {
        GameEventValueChecker value;
        ~Values() { delete[] value.mValues; }
    } owner;
    auto& values = owner.value;
    values.setValue(book_name.c_str(), 0x1234);
    Bytes wire(100);
    require(serialize_save_chunk(values, wire.data(), wire.size()) == 100, "actual original25-value payload");
    require(wire[44] == 0xf8 && wire[45] == 0xc3 && wire[46] == 0x12 && wire[47] == 0x34,
            "actual original Japanese value serializer emits retail ID and endian bytes");
    require(deserialize_save_chunk(values, wire.data(), wire.size()) == 0 && values.getValue(book_name.c_str()) == 0x1234,
            "actual original Japanese value roundtrip retains authored EOF loop");
    const Bytes one{0xb4,0xca,0x12,0x34};
    require(deserialize_save_chunk(values, one.data(), one.size()) == 0 && values.getValue("MissNum") == 0x1234,
            "one actual VLE1 pair survives two original loop iterations");
    const Bytes two{0xb4,0xca,0,7, 0x72,0xdb,0,9};
    require(deserialize_save_chunk(values, two.data(), two.size()) == 0 && values.getValue("MissNum") == 7 &&
            values.getValue("MissPointForLetter") == 9, "two actual VLE1 pairs survive four original loop iterations");
    const Bytes unknown{0xff,0xff,0,1, 0xb4,0xca,0,11};
    require(deserialize_save_chunk(values, unknown.data(), unknown.size()) == 1 && values.getValue("MissNum") == 11,
            "original VLE1 retains earlier unknown-ID error after final valid pair and EOF");
    require(MR::getHashCode("絵本既読章") == 0xba504313 && MR::getHashCode(book_name.c_str()) == 0x1097f8c3,
            "save serialization cannot change the raw hash's Game or host byte behavior");
}
} // namespace

int main() {
    try {
        play(); flat_formats(); paths(); galaxies(); hashes_and_actual_values();
        std::cout << "[ok] " << checks << " checks: actual PLAY/VLE1, all original FLG1 sorted identities, six-format bytes and malformed boundaries\n";
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
