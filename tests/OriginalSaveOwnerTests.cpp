#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/SpinDriverPathStorage.hpp"
#include "Game/System/StarPieceAlmsStorage.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "resource/TextEncoding.hpp"

#include <aurora/endian.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void test_galaxy_schema(GameDataAllGalaxyStorage& galaxies) {
    require(galaxies.getGalaxyNum() > 1, "GALA schema test needs the real galaxy catalog");
    auto* galaxy = galaxies.getGalaxyStorage(0);
    auto* untouched = galaxies.getGalaxyStorage(1);
    untouched->mMaxCoinNum[0] = 777;

    // Reordered fields, an unknown field, unaligned coin data, and a duplicate
    // name descriptor exercise the original first-match schema lookup.
    constexpr std::size_t header_size = 30;
    constexpr u16 record_size = 25;
    auto bytes = std::vector<u8>(header_size + record_size, 0xa5);
    auto put16 = [&](std::size_t offset, u16 value) { aurora::endian::write_u16(bytes.data() + offset, value); };
    put16(0, 1);
    put16(2, 6);
    put16(4, record_size);
    auto attribute = [&](unsigned index, const char* name, u16 offset) {
        put16(6 + index * 4, static_cast<u16>(MR::getHashCode(name)));
        put16(8 + index * 4, offset);
    };
    attribute(0, "mMaxCoinNum", 1);
    attribute(1, "futureField", record_size);
    attribute(2, "mFirstPlayFlag", 17);
    attribute(3, "mGalaxyName", 20);
    attribute(4, "mPowerStarFlag", 18);
    // This later descriptor has no room for a name; the first match wins.
    attribute(5, "mGalaxyName", record_size);
    bytes[header_size + 17] = 0x42;
    bytes[header_size + 18] = 0x35;
    put16(header_size + 20, static_cast<u16>(MR::getHashCode(galaxy->mGalaxyName)));
    for (unsigned index = 0; index < 8; ++index) put16(header_size + 1 + index * 2, 0x120 + index * 0x101);
    require(galaxies.deserialize(bytes.data(), bytes.size()) == 0 && galaxy->mPowerStarOwnedFlags == 0x35 &&
                galaxy->mAlreadyVisitedFlags == 0x42, "GALA reordered and duplicate descriptors retain original lookup behavior");
    for (unsigned index = 0; index < 8; ++index)
        require(galaxy->mMaxCoinNum[index] == 0x120 + index * 0x101, "GALA reads unaligned big-endian coin records");
    require(untouched->mMaxCoinNum[0] == 777, "GALA leaves absent galaxy records untouched");

    std::array<u8, 4096> serialized{};
    require(galaxies.serialize(serialized.data(), serialized.size()) == 22 + galaxies.getGalaxyNum() * 20,
            "GALA serializes the original twenty-byte records");
    constexpr std::array<u8, 20> golden_header{0, 4, 0, 20, 0x82, 8, 0, 0, 0x21, 0x96, 0, 2,
                                             0xd4, 0x23, 0, 3, 0x81, 0x7e, 0, 4};
    require(std::equal(golden_header.begin(), golden_header.end(), serialized.begin() + 2),
            "GALA emits the original big-endian descriptor hashes, offsets and record width");
    require(aurora::endian::read_u16(serialized.data()) == galaxies.getGalaxyNum() && serialized[24] == 0x35 && serialized[25] == 0x42,
            "GALA emits the catalog count and original one-byte flag fields");
    for (unsigned index = 0; index < 8; ++index)
        require(serialized[26 + index * 2] == (0x120 + index * 0x101) >> 8 &&
                    serialized[27 + index * 2] == ((0x120 + index * 0x101) & 0xff), "GALA emits all eight Wii coin scalars");

    auto reject = [&](const std::vector<u8>& invalid) {
        require(galaxies.deserialize(invalid.data(), invalid.size()) == -1, "GALA rejects malformed schemas");
        require(galaxy->mPowerStarOwnedFlags == 0x35 && galaxy->mAlreadyVisitedFlags == 0x42 &&
                    untouched->mMaxCoinNum[0] == 777, "GALA rejects malformed schemas before mutating flags or other records");
        for (unsigned index = 0; index < 8; ++index)
            require(galaxy->mMaxCoinNum[index] == 0x120 + index * 0x101, "Rejected GALA schemas leave every coin value unchanged");
    };
    auto corrupt16 = [&](std::size_t offset, u16 value) {
        auto invalid = bytes;
        aurora::endian::write_u16(invalid.data() + offset, value);
        reject(invalid);
    };
    reject(std::vector<u8>(bytes.begin(), bytes.begin() + 5));
    reject(std::vector<u8>(bytes.begin(), bytes.end() - 1));
    corrupt16(0, 2); // Record count exceeds the payload.
    corrupt16(2, 0xffff); // Descriptor table exceeds the payload.
    corrupt16(4, 0); // A nonempty record table cannot have zero-sized records.
    corrupt16(8, 10); // Sixteen coin bytes extend past the record.
    corrupt16(12, 26); // Even unknown descriptors must lie within the record.
    corrupt16(20, 1); // Name and coin fields overlap.
    auto missing_name = bytes;
    aurora::endian::write_u16(missing_name.data() + 18, 0);
    aurora::endian::write_u16(missing_name.data() + 26, 0);
    reject(missing_name);

    const auto name_hash = static_cast<u16>(MR::getHashCode(galaxy->mGalaxyName));
    const auto attribute_hash = static_cast<u16>(MR::getHashCode("mGalaxyName"));
    const std::array<u8, 12> legacy{0, 1, 0, 1, 0, 2, static_cast<u8>(attribute_hash >> 8),
        static_cast<u8>(attribute_hash), 0, 0, static_cast<u8>(name_hash >> 8), static_cast<u8>(name_hash)};
    require(galaxies.deserialize(legacy.data(), legacy.size()) == 1 && galaxy->mPowerStarOwnedFlags == 0 &&
                galaxy->mAlreadyVisitedFlags == 0, "Legacy name-only GALA records retain recoverable status and default missing flags");
    for (auto coin : galaxy->mMaxCoinNum) require(coin == 0, "Legacy GALA records default missing coin fields");
    const std::array<u8, 6> empty{};
    require(galaxies.deserialize(empty.data(), empty.size()) == 0 && untouched->mMaxCoinNum[0] == 777,
            "Empty GALA schemas need no name descriptor and preserve existing records");
    galaxies.initializeData();
}

struct UserFileSnapshot {
    UserFile& file;
    const std::string game_name;
    const std::string config_name;
    const GameDataPlayerStatus player;
    const bool game_corrupted;
    const bool config_corrupted;
    std::array<u8, 4096> game{};
    std::array<u8, 256> config{};

    explicit UserFileSnapshot(UserFile& source)
        : file(source), game_name(source.getGameDataName()), config_name(source.getConfigDataName()),
          player(*source.mGameDataHolder->mPlayerStatus), game_corrupted(source.mIsGameDataCorrupted),
          config_corrupted(source.mIsConfigDataCorrupted) {
        file.makeGameDataBinary(game.data(), game.size());
        file.makeConfigDataBinary(config.data(), config.size());
    }

    ~UserFileSnapshot() {
        file.loadFromGameDataBinary(game_name.c_str(), game.data(), game.size());
        file.loadFromConfigDataBinary(config_name.c_str(), config.data(), config.size());
        // PLAY loading intentionally resets active lives and interprets saved
        // lives as supply. Preserve those runtime-only values as well.
        *file.mGameDataHolder->mPlayerStatus = player;
        file.mIsGameDataCorrupted = game_corrupted;
        file.mIsConfigDataCorrupted = config_corrupted;
    }
};
}

int main() {
    return smgpc::test::run_stage_resource_process("original-save-owner", [] {
        auto* system = SingletonHolder<GameSystem>::get();
        require(system && system->mSequenceDirector && system->mSequenceDirector->mSaveDataHandleSequence,
                "Save assertions require the complete original process sequence");
        auto& sequence = *system->mSequenceDirector->mSaveDataHandleSequence;
        auto& current_file = *sequence.getCurrentUserFile();
        auto& backup_file = *sequence.getBackupUserFile();
        require(&current_file != &backup_file && current_file.mGameDataHolder != backup_file.mGameDataHolder,
                "The original save sequence owns independent current and backup files");
        UserFileSnapshot restore_current(current_file);
        UserFileSnapshot restore_backup(backup_file);
        auto& current = *current_file.mGameDataHolder;
        const auto& backup = *backup_file.mGameDataHolder;
        current.mPlayerStatus->mStockedStarPiece = 0;
        current.mStarPieceAlmsStorage->initializeData();
        test_galaxy_schema(*current.mAllGalaxyStorage);
        require(!restore_current.config_name.empty(), "The original process selects an actual config identity");

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
        sequence.backupCurrentUserFile();
        require(!backup_file.mIsGameDataCorrupted && !backup_file.mIsConfigDataCorrupted &&
                    std::string_view(backup_file.getConfigDataName()) == restore_current.config_name &&
                    std::string_view(backup_file.getGameDataName()) == restore_current.game_name,
                "Original snapshot preserves the process-selected game and config identities");
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
        current_file.loadFromGameDataBinary(restore_current.game_name.c_str(), file.data(), size);
        require(current_file.mIsGameDataCorrupted && current.mPlayerStatus->mStoryProgress == 0,
                "Malformed later chunks must not partially apply earlier save state");
        std::cout << "PASS six original chunks, 188 cached flag lookups, 14 story thresholds, Wii bytes and atomic validation\n";
    });
}
