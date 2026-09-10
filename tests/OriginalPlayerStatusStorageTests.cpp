#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "compat/SaveChunkEncoding.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
using Payload = std::array<u8, 7>;

Payload encode_status(const GameDataPlayerStatus& status) {
    Payload payload{};
    require(smgpc::compat::serialize_save_chunk(status, payload.data(), payload.size()) == payload.size(),
            "the original PLAY chunk stores exactly seven Wii bytes");
    return payload;
}

void require_initialized(const GameDataPlayerStatus& status) {
    require(status.mPlayerLeft == 4 && status.mStockedStarPiece == 0 && status.mStoryProgress == 0 &&
                !status.isPlayerLeftSupply(),
            "initialization resets all original PLAY fields, including private saved-life supply");
}

void test_original_payload_and_load() {
    GameDataPlayerStatus status;
    require_initialized(status);
    require(status.getSignature() == 0x504c4159 && status.makeHeaderHashCode() == 0x27c90f,
            "the original object supplies the retail PLAY signature and header hash");
    status.mStoryProgress = 0x2a;
    status.mStockedStarPiece = 0x01020304;
    status.mPlayerLeft = 0x1234;
    const Payload expected{0x2a, 1, 2, 3, 4, 0x12, 0x34};
    require(encode_status(status) == expected, "the save boundary preserves the literal Wii PLAY payload");

    Payload native{};
    require(status.serialize(native.data(), native.size()) == native.size(),
            "the unchanged original serializer writes its full native payload");
    u32 native_stock;
    u16 native_lives;
    std::memcpy(&native_stock, native.data() + 1, sizeof(native_stock));
    std::memcpy(&native_lives, native.data() + 5, sizeof(native_lives));
    require(native[0] == 0x2a && native_stock == 0x01020304 && native_lives == 0x1234,
            "raw JSU writes preserve native scalar order; only the explicit save boundary converts bytes");
    GameDataPlayerStatus raw_loaded;
    require(raw_loaded.deserialize(native.data(), native.size()) == 0 && raw_loaded.mStoryProgress == 0x2a &&
                raw_loaded.mStockedStarPiece == 0x01020304 && raw_loaded.mPlayerLeft == 4 && raw_loaded.isPlayerLeftSupply(),
            "the original raw deserializer consumes native scalar bytes and applies original life resupply");

    for (u32 length = 0; length <= 9; ++length) {
        std::array<u8, 11> guarded;
        guarded.fill(0xcd);
        const auto copied = smgpc::compat::serialize_save_chunk(status, guarded.data() + 1, length);
        require(copied == std::min<u32>(length, 7), "short output retains the original stream position");
        require(guarded.front() == 0xcd &&
                    std::all_of(guarded.begin() + copied + 1, guarded.end(), [](u8 byte) { return byte == 0xcd; }),
                "short output preserves every byte outside its written prefix");
        require(std::memcmp(guarded.data() + 1, expected.data(), copied) == 0,
                "every short output is the literal Wii payload prefix");
    }
    require(smgpc::compat::serialize_save_chunk(status, nullptr, 7) == 0, "null output writes nothing");
    native.fill(0xcd);
    require(smgpc::compat::serialize_save_chunk(status, native.data(), std::numeric_limits<u32>::max()) == 0 &&
                std::all_of(native.begin(), native.end(), [](u8 byte) { return byte == 0xcd; }),
            "an extent outside the original signed stream range writes nothing");

    require(smgpc::compat::deserialize_save_chunk(status, expected.data(), expected.size()) == 0 &&
                status.mPlayerLeft == 4 && status.mStoryProgress == 0x2a && status.mStockedStarPiece == 0x01020304 &&
                status.isPlayerLeftSupply(),
            "original load restores story and stock, loads saved lives as supply, and resets active lives to four");
    status.offPlayerLeftSupply();
    require(!status.isPlayerLeftSupply(), "the original method clears the actual private supply field");
    for (u8 saved_lives : {u8(9), u8(10)}) {
        const Payload payload{5, 0, 0, 0x12, 0x34, 0, saved_lives};
        require(smgpc::compat::deserialize_save_chunk(status, payload.data(), payload.size()) == 0 &&
                    status.isPlayerLeftSupply() == (saved_lives >= 10) && status.mPlayerLeft == 4,
                "the original resupply threshold remains ten independent of fresh active lives");
    }

    const std::array<u8, 9> extended{0x2a, 1, 2, 3, 4, 0x12, 0x34, 0xee, 0xff};
    for (u32 length = 0; length <= extended.size(); ++length) {
        require(smgpc::compat::deserialize_save_chunk(status, extended.data(), length) == 0,
                "every legacy prefix length retains the original successful load result");
        u32 stocked = 0;
        for (u32 i = 1; i < std::min<u32>(length, 5); ++i) stocked |= u32(expected[i]) << ((4 - i) * 8);
        require(status.mStockedStarPiece == stocked && status.mStoryProgress == (length ? expected[0] : 0) &&
                    status.mPlayerLeft == 4 && status.isPlayerLeftSupply() == (length >= 6),
                "partial input replaces original most-significant bytes, keeps defaults, and ignores trailing bytes");
    }
    require(smgpc::compat::deserialize_save_chunk(status, nullptr, 7) == 0, "null input follows initialized prefix loading");
    require_initialized(status);
    require(smgpc::compat::deserialize_save_chunk(status, expected.data(), std::numeric_limits<u32>::max()) == 0,
            "an extent outside the original signed stream range is an empty load");
    require_initialized(status);

    status.mStoryProgress = 0xff;
    status.mStockedStarPiece = 0xfedcba98;
    status.mPlayerLeft = 0xfedc;
    const Payload wide{0xff, 0xfe, 0xdc, 0xba, 0x98, 0xfe, 0xdc};
    require(encode_status(status) == wide, "all scalar high bits survive the original serializer and save conversion");
    require(smgpc::compat::deserialize_save_chunk(status, wide.data(), wide.size()) == 0 &&
                status.mStoryProgress == 0xff && status.mStockedStarPiece == 0xfedcba98 &&
                status.mPlayerLeft == 4 && status.isPlayerLeftSupply(),
            "wide saved fields are loaded without gameplay clamping or sign extension");
    status.initializeData();
    require_initialized(status);
}

void test_chunk_container_and_original_methods() {
    GameDataPlayerStatus source;
    const Payload saved{15, 0, 0, 0x23, 0x45, 0, 12};
    require(smgpc::compat::deserialize_save_chunk(source, saved.data(), saved.size()) == 0,
            "source status is loaded by the original deserializer");
    source.addPlayerLeft(8);
    require(source.getPlayerLeft() == 12 && source.isPlayerLeftSupply(),
            "active lives and loaded private supply remain separate original fields");

    struct Container {
        BinaryDataChunkHolder holder{32, 1};
        ~Container() { delete[] static_cast<u8*>(holder.mData); delete[] holder.mChunks; }
    } container;
    container.holder.addChunk(&source);
    std::array<u8, 32> bytes{};
    const auto size = container.holder.makeFileBinary(bytes.data(), bytes.size());
    constexpr std::array<u8, 16> header{1, 1, 0, 0, 'P', 'L', 'A', 'Y', 0, 0x27, 0xc9, 0x0f, 0, 0, 0, 19};
    require(size == 23 && std::memcmp(bytes.data(), header.data(), header.size()) == 0 &&
                std::memcmp(bytes.data() + header.size(), saved.data(), saved.size()) == 0,
            "the chunk container dispatches the actual PLAY object through the explicit Wii encoding boundary");

    GameDataPlayerStatus destination;
    container.holder.mChunks[0] = &destination;
    require(container.holder.loadFromFileBinary(bytes.data(), size) && destination.mPlayerLeft == 4 &&
                destination.isPlayerLeftSupply() && destination.mStockedStarPiece == 0x2345 &&
                destination.mStoryProgress == 15 && source.mPlayerLeft == 12,
            "container load reaches the destination's original resupply behavior without changing its source");
    destination.addPlayerLeft(1000);
    destination.addStockedStarPiece(10000);
    require(destination.getPlayerLeft() == 99 && destination.mStockedStarPiece == 9999,
            "original status methods retain their upper clamps");
    destination.addPlayerLeft(-1000);
    destination.addStockedStarPiece(-10000);
    require(destination.getPlayerLeft() == 0 && destination.mStockedStarPiece == 0,
            "original status methods retain their lower clamps");
    destination.initializeData();
    require_initialized(destination);
}
}  // namespace

int main() {
    try {
        test_original_payload_and_load();
        test_chunk_container_and_original_methods();
        std::cout << "[ok] original PLAY native serialization, Wii golden bytes, partial loads, resupply, and container dispatch\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original PLAY: " << error.what() << '\n';
        return 1;
    }
}
