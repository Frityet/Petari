#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/SpinDriverPathStorage.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/GameDataSession.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/endian.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

namespace aurora { extern AuroraConfig g_config; }
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "Original save ownership needs the real scenario catalog");
        struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime resources;
        smgpc::runtime::DvdFileSystemService dvd({});
        smgpc::runtime::ArchiveMountService mounts(dvd);
        auto catalog = std::make_shared<smgpc::runtime::ScenarioCatalogOwnership>(
            resources.host_heaps(), resources.budget().scenario_catalog_bytes, mounts);
        smgpc::compat::GameDataSession session(3, resources, catalog);
        auto& current = session.holder();
        const auto& backup = session.scene_start_holder();
        require(std::string_view(session.user_file().getConfigDataName()) == "config3", "Selected config identity");

        std::set<u16> hashes;
        for (int index = 0; index < GameEventFlagTable::getTableSize(); ++index) {
            const auto* flag = GameEventFlagTable::getFlag(index);
            require(GameEventFlagTable::findFlag(flag->mName) == flag, "Every original flag must survive cached lookup");
            require(hashes.insert(static_cast<u16>(MR::getHashCode(flag->mName))).second,
                    "Original flag identifiers must have distinct sixteen-bit hashes");
        }
        require(hashes.size() == 188, "Retail flag count");
        const auto boo = smgpc::resource::encode_cp932("テレサマリオ初変身");
        require(MR::getHashCode(boo.c_str()) == 0x9e8d278e && MR::getHashCode("CocoonExGalaxy") == 0xfc46389a,
                "Fixed Wii hashes for the two formerly colliding UTF-8 names");
        require(GameEventFlagTable::findFlag(boo.c_str()) && GameEventFlagTable::findFlag("CocoonExGalaxy"),
                "Both formerly colliding flags must be addressable");
        current.tryOnGameEventFlag(boo.c_str());
        require(current.isOnGameEventFlag(boo.c_str()), "Original Japanese flag write");

        const auto* table = current.mMapInfo;
        require(table->getNumEntries() == 14, "Actual embedded story BCSV row count");
        for (int row = 0; row < table->getNumEntries(); ++row) {
            const char* name = nullptr;
            u32 progress = 0;
            require(table->getValue(row, "name", &name) && table->getValue(row, "progress", &progress),
                    "Original story fields must be readable");
            current.followStoryEventByName(name);
            require(current.mPlayerStatus->mStoryProgress == progress && current.isPassedStoryEvent(name),
                    "Every original story row must route through the unchanged byte comparison");
            if (progress) {
                current.mPlayerStatus->mStoryProgress = progress - 1;
                require(!current.isPassedStoryEvent(name), "Each story threshold must reject its preceding step");
            }
        }
        current.followStoryEventByName(smgpc::resource::encode_cp932("スピン権利").c_str());
        current.setPictureBookChapterAlreadyRead(7);
        current.addStockedStarPiece(1234);
        for (int index = 0; index < 16; ++index)
            current.addStarPieceGivingToTicoSeed(index, index * 17 + 1);
        auto* paths = current.mSpinDriverPathStorage;
        require(paths->mGalaxyStorage.size() > 2, "Actual scenario catalog supplies multiple path galaxies");
        const auto* path_galaxy = paths->mGalaxyStorage[0];
        f32 range = -1.0F;
        const auto path_index = current.setupSpinDriverPathStorage(path_galaxy->mGalaxyName, 1, 0, 2, &range);
        require(path_index >= 0 && range == 0.0F, "Original path allocation");
        current.updateSpinDriverPathStorage(path_galaxy->mGalaxyName, 1, path_index, 0.5F);
        session.store_scene_start();
        require(backup.mPlayerStatus->mStoryProgress == 15 && backup.getStockedStarPieceNum() == 1234 &&
                backup.getPictureBookChapterAlreadyRead() == 7 && backup.isOnGameEventFlag(boo.c_str()),
                "Actual PLAY, FLG1 and VLE1 payloads survive a file snapshot");
        for (int index = 0; index < 16; ++index)
            require(backup.getStarPieceNumGivingToTicoSeed(index) == index * 17 + 1, "All PCE1 native scalars survive Wii byte order");
        auto* restored = backup.mSpinDriverPathStorage->findFromGalaxy(path_galaxy->mGalaxyName);
        require(restored && restored != path_galaxy && restored->getScenarioStorage(1).mOneStorage.size() == 1 &&
                restored->getScenarioStorage(1).mOneStorage[0].mDrawRange == 0.5F,
                "SPN1 reconstructs a distinct original path record at the authored precision");

        std::array<u8, 4096> file{};
        const auto size = current.makeFileBinary(file.data(), file.size());
        constexpr std::array<u32, 6> signatures{0x504c4159, 0x464c4731, 0x50434531, 0x53504e31, 0x564c4531, 0x47414c41};
        require(file[0] == 1 && file[1] == signatures.size(), "Original six-chunk file header");
        std::size_t offset = 4;
        std::size_t path_offset = 0;
        for (const auto signature : signatures) {
            require(offset + 12 <= size && aurora::endian::read_u32(file.data() + offset) == signature,
                    "Original chunk order and big-endian signatures");
            const auto chunk_size = aurora::endian::read_u32(file.data() + offset + 8);
            require(chunk_size >= 12 && chunk_size <= size - offset, "Bounded original chunk size");
            if (signature == 0x53504e31) path_offset = offset;
            if (signature == 0x504c4159)
                require(file[offset + 12] == 15 && aurora::endian::read_u32(file.data() + offset + 13) == 1234,
                        "PLAY fixed Wii byte layout");
            offset += chunk_size;
        }
        require(offset == size && path_offset != 0, "Complete original file extent");
        // Corrupt the first SPN1 galaxy block extent after earlier valid chunks.
        // Validation must reject the whole file before mutating PLAY/FLG1/PCE1.
        file[path_offset + 12 + 3] = 0xff;
        file[path_offset + 12 + 4] = 0xff;
        current.followStoryEventByName(smgpc::resource::encode_cp932("ゲーム開始直後").c_str());
        session.user_file().loadFromGameDataBinary("mario3", file.data(), size);
        require(session.user_file().mIsGameDataCorrupted && current.mPlayerStatus->mStoryProgress == 0,
                "Malformed later chunks must not partially apply earlier save state");
        std::cout << "PASS six original chunks, 188 cached flag lookups, 14 story thresholds, Wii bytes and atomic validation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Original save owner: " << error.what() << '\n';
        return 1;
    }
}
