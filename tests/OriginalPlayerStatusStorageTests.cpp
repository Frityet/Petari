#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
using Payload = std::array<u8, 7>;

Payload serialize(const GameDataPlayerStatus& status) {
    Payload payload{};
    require(status.serialize(payload.data(), payload.size()) == payload.size(), "PLAY stores exactly seven bytes");
    return payload;
}

void test_original_payload_and_load() {
    smgpc::compat::GameDataSession session(2);
    auto& holder = session.holder();
    auto* status = holder.mPlayerStatus;
    require(status != nullptr && GameDataFunction::getCurrentGameDataHolder() == &holder,
            "selected profile publishes its actual original player-status owner");
    status->mStoryProgress = 0x2a;
    status->mStockedStarPiece = 0x01020304;
    status->mPlayerLeft = 0x1234;
    const Payload expected{0x2a, 1, 2, 3, 4, 0x12, 0x34};
    require(serialize(*status) == expected, "actual virtual PLAY serializer retains original Wii byte order");
    for (u32 length = 0; length <= 9; ++length) {
        std::array<u8, 11> guarded;
        guarded.fill(0xcd);
        const auto copied = status->serialize(guarded.data() + 1, length);
        require(copied == std::min<u32>(length, 7), "short output retains original stream position");
        require(guarded.front() == 0xcd && guarded[copied + 1] == 0xcd, "short output preserves its boundary canaries");
        require(std::memcmp(guarded.data() + 1, expected.data(), copied) == 0, "short output is the Wii payload prefix");
    }
    require(status->serialize(nullptr, 7) == 0, "null byte-stream output writes nothing");

    require(status->deserialize(expected.data(), expected.size()) == 0 && status->getPlayerLeft() == 4 &&
                holder.getPlayerLeft() == 4 && status->mStoryProgress == 0x2a &&
                status->mStockedStarPiece == 0x01020304 && holder.isPlayerLeftSupply(),
            "original load restores story/stock, loads saved lives as supply and resets playable lives to4");
    holder.offPlayerLeftSupply();
    require(!status->isPlayerLeftSupply(), "holder and original object share the same supply field");
    for (u8 saved_lives : {u8(9), u8(10)}) {
        Payload payload{5, 0, 0, 0x12, 0x34, 0, saved_lives};
        status->deserialize(payload.data(), payload.size());
        require(holder.isPlayerLeftSupply() == (saved_lives >= 10) && holder.getPlayerLeft() == 4,
                "original resupply threshold remains10 independent from fresh playable lives");
    }
    for (u32 length = 0; length <= expected.size(); ++length) {
        status->deserialize(expected.data(), length);
        u32 stocked = 0;
        for (u32 i = 1; i < std::min<u32>(length, 5); ++i) stocked |= u32(expected[i]) << ((4 - i) * 8);
        require(status->mStockedStarPiece == stocked && status->mStoryProgress == (length ? expected[0] : 0) &&
                    holder.getPlayerLeft() == 4 && holder.isPlayerLeftSupply() == (length >= 6),
                "truncated input replaces the original most-significant bytes and preserves initialization defaults");
    }
    status->deserialize(nullptr, 7);
    require(holder.getPlayerLeft() == 4 && holder.getStockedStarPieceNum() == 0 && !holder.isPlayerLeftSupply(),
            "null input retains original initialized values");
}

void test_chunk_container_and_owner_copy() {
    smgpc::compat::GameDataSession first(1);
    auto& source = first.holder();
    auto* source_status = source.mPlayerStatus;
    const Payload saved{15, 0, 0, 0x23, 0x45, 0, 12};
    source_status->deserialize(saved.data(), saved.size());
    source.addPlayerLeft(8);
    require(source.getPlayerLeft() == 12 && source.isPassedStoryEvent("スピン権利") && source.isPlayerLeftSupply(),
            "original loaded state is visible to life and story queries");

    struct Container {
        BinaryDataChunkHolder holder{32, 1};
        ~Container() { delete[] static_cast<u8*>(holder.mData); delete[] holder.mChunks; }
    } container;
    container.holder.addChunk(source_status);
    std::array<u8, 32> bytes{};
    const auto size = container.holder.makeFileBinary(bytes.data(), bytes.size());
    constexpr std::array<u8, 16> header{1, 1, 0, 0, 'P', 'L', 'A', 'Y', 0, 0x27, 0xc9, 0x0f, 0, 0, 0, 19};
    require(size == 23 && std::memcmp(bytes.data(), header.data(), header.size()) == 0 &&
                std::memcmp(bytes.data() + header.size(), saved.data(), saved.size()) == 0,
            "real BinaryDataChunkHolder owns a Wii PLAY header and invokes the selected object's virtual serializer");

    {
        smgpc::compat::GameDataSession second(6);
        auto& destination = second.holder();
        auto* destination_status = destination.mPlayerStatus;
        require(destination_status != source_status, "simultaneous profiles own different original status objects");
        smgpc::compat::game_data::copy_holder_state(destination, source);
        require(destination.mPlayerStatus == destination_status && destination.getPlayerLeft() == 12 &&
                    destination.isPlayerLeftSupply() && destination.getStockedStarPieceNum() == 0x2345,
                "profile copy preserves destination owner identity and all original in-memory fields");
        source.offPlayerLeftSupply();
        source.addPlayerLeft(-2);
        require(destination.isPlayerLeftSupply() && destination.getPlayerLeft() == 12,
                "copied status and private supply field do not alias their source");
        destination.resetAllData();
        require(destination.mPlayerStatus == destination_status && destination.getPlayerLeft() == 4 &&
                    destination.getStockedStarPieceNum() == 0 && !destination.isPlayerLeftSupply() &&
                    smgpc::compat::game_data::holder_story_progress(destination) == 0,
                "reset preserves original object identity and resets all PLAY fields");
        container.holder.mChunks[0] = destination_status;
        require(container.holder.loadFromFileBinary(bytes.data(), size) && destination.getPlayerLeft() == 4 &&
                    destination.isPlayerLeftSupply() && destination.getStockedStarPieceNum() == 0x2345,
                "real chunk load reaches original saved-life resupply behavior through the virtual boundary");
        destination.addPlayerLeft(1000);
        destination.addStockedStarPiece(10000);
        require(destination.getPlayerLeft() == 99 && destination.getStockedStarPieceNum() == 9999,
                "original status methods retain upper clamps");
        destination.addPlayerLeft(-1000);
        destination.addStockedStarPiece(-10000);
        require(destination.getPlayerLeft() == 0 && destination.getStockedStarPieceNum() == 0,
                "original status methods retain lower clamps");
    }
    require(GameDataFunction::getCurrentGameDataHolder() == &source && source.getPlayerLeft() == 10,
            "nested profile retirement restores the still-live original outer owner");
}

void test_generations_and_scene_heap_escape() {
    const auto count = smgpc::compat::game_data::holder_state_count();
    {
        GameDataHolder retiring(nullptr);
        require(retiring.mPlayerStatus != nullptr, "a standalone selected holder also publishes its actual status owner");
        smgpc::compat::game_data::destroy_holder_state(retiring);
        require(retiring.mPlayerStatus == nullptr && smgpc::compat::game_data::holder_state_count() == count,
                "explicit typed owner retirement clears the borrowed original pointer");
    }
    auto heaps = smgpc::compat::JkrHeapRuntime::create(8 * 1024 * 1024);
    for (unsigned generation = 0; generation < 3; ++generation) {
        auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 4096);
        std::weak_ptr<smgpc::compat::JkrAllocationDomain> weak = domain;
        std::optional<smgpc::compat::GameDataSession> profile;
        {
            smgpc::compat::JkrAllocationScope scene(domain);
            profile.emplace(3);
            require(JKRHeap::findFromRoot(profile->holder().mPlayerStatus) == nullptr,
                    "process selected-profile status does not belong to the current scene arena");
        }
        domain.reset();
        require(weak.expired(), "selected profile does not retain the unrelated scene generation");
        require(profile->holder().getPlayerLeft() == 4 && profile->holder().mPlayerStatus != nullptr,
                "original status remains valid after scene heap retirement");
        profile->holder().addPlayerLeft(generation + 1);
        profile.reset();
        require(smgpc::compat::game_data::holder_state_count() == count,
                "typed selected-profile teardown removes its concrete status owner every generation");
    }
}
}  // namespace

int main() {
    try {
        test_original_payload_and_load();
        test_chunk_container_and_owner_copy();
        test_generations_and_scene_heap_escape();
        std::cout << "[ok] original PLAY payload/load, actual selected-profile copy/reset, and typed lifetime\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original PLAY: " << error.what() << '\n';
        return 1;
    }
}
