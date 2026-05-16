#include "game/Game/System/NANDManager.hpp"
#include "game/Game/System/SaveDataHandleSequence.hpp"
#include "game/Game/System/SaveDataHandler.hpp"
#include "game/Game/System/SysConfigFile.hpp"
#include "game/Game/System/UserFile.hpp"
#include "tests/TestFilesystem.hpp"
#include "tests/TestHarness.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

    [[nodiscard]] std::vector< unsigned char > read_binary(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        $pc_port_require(stream.is_open());
        return std::vector< unsigned char >((std::istreambuf_iterator< char >(stream)), std::istreambuf_iterator< char >());
    }

    void update_sequence_until_done(SaveDataHandleSequence& sequence, int maxFrames = 32) {
        for (int i = 0; i < maxFrames && sequence.isActive(); ++i) {
            sequence.update();
        }
        $pc_port_require(!sequence.isActive());
        $pc_port_require_eq(sequence._24, 2);
    }

    $test("SaveDataHandleSequence creates user files through SaveDataHandler") {
        smgpc::test::TempDirectory temp{};
        setenv("SMGPC_SAVE_DIR", temp.path().string().c_str(), 1);

        SaveDataHandleSequence sequence{};
        sequence.initAfterResourceLoaded();
        sequence.startPreLoad();
        update_sequence_until_done(sequence);

        sequence.startCreateUserFile(3);
        update_sequence_until_done(sequence);

        SaveDataHandleSequence reloaded{};
        reloaded.initAfterResourceLoaded();
        reloaded.startPreLoad();
        update_sequence_until_done(reloaded);

        UserFile slot{};
        reloaded.restoreUserFile(&slot, 3);
        $pc_port_require(slot.isCreated());
    }

    $test("SaveDataHandleSequence stores selected fellow icon through save-all path") {
        smgpc::test::TempDirectory temp{};
        setenv("SMGPC_SAVE_DIR", temp.path().string().c_str(), 1);

        SaveDataHandleSequence sequence{};
        sequence.initAfterResourceLoaded();
        sequence.startPreLoad();
        update_sequence_until_done(sequence);

        sequence.startCreateUserFile(2);
        update_sequence_until_done(sequence);

        const u32 peachIconId = 5U;
        sequence.storeMiiOrIconId(2, nullptr, &peachIconId);
        sequence.startSaveAll();
        update_sequence_until_done(sequence);

        SaveDataHandleSequence reloaded{};
        reloaded.initAfterResourceLoaded();
        reloaded.startPreLoad();
        update_sequence_until_done(reloaded);

        UserFile slot{};
        reloaded.restoreUserFile(&slot, 2);
        $pc_port_require(slot.isCreated());

        u32 storedIconId = 0U;
        $pc_port_require(slot.getIconId(&storedIconId));
        $pc_port_require_eq(storedIconId, peachIconId);
    }

    void update_until_done(SaveDataHandler& handler) {
        for (int i = 0; i < 8 && !handler.isDone(); ++i) {
            handler.update();
        }
        $pc_port_require(handler.isDone());
    }

    [[nodiscard]] unsigned read_u32_be(const std::vector< unsigned char >& bytes, std::size_t offset) {
        return (static_cast< unsigned >(bytes[offset]) << 24U) | (static_cast< unsigned >(bytes[offset + 1U]) << 16U) |
               (static_cast< unsigned >(bytes[offset + 2U]) << 8U) | static_cast< unsigned >(bytes[offset + 3U]);
    }

    void require_game_data_default_chunks(const std::vector< unsigned char >& bytes, std::size_t offset) {
        constexpr unsigned expected_signatures[]{
            0x504C4159U,  // PLAY
            0x464C4731U,  // FLG1
            0x50434531U,  // PCE1
            0x53504E31U,  // SPN1
            0x564C4531U,  // VLE1
            0x47414C41U,  // GALA
        };
        constexpr unsigned expected_hashes[]{
            0x0027C90FU, 0x65020442U, 0xF5DE1DC0U, 0x12345679U, 0x564C4531U, 0xBF0640EEU,
        };
        constexpr unsigned expected_sizes[]{
            0x13U, 0x7EU, 0x2CU, 0x28FU, 0x70U, 0x36AU,
        };

        $pc_port_require_eq(static_cast< unsigned >(bytes[offset]), 1U);
        $pc_port_require_eq(static_cast< unsigned >(bytes[offset + 1U]), 6U);

        std::size_t chunk_offset = offset + 4U;
        for (std::size_t i = 0; i < std::size(expected_signatures); ++i) {
            $pc_port_require_eq(read_u32_be(bytes, chunk_offset), expected_signatures[i]);
            $pc_port_require_eq(read_u32_be(bytes, chunk_offset + 4U), expected_hashes[i]);
            $pc_port_require_eq(read_u32_be(bytes, chunk_offset + 8U), expected_sizes[i]);
            chunk_offset += expected_sizes[i];
        }
        $pc_port_require_eq(chunk_offset, offset + 0x72AU);
    }

}  // namespace

$test("GameDataHolder preserves loaded Wii game-data chunks") {
    UserFile file{};
    std::vector< u8 > original(SaveDataHandler::getEnoughtTempBufferSize());
    file.makeGameDataBinary(original.data(), static_cast< u32 >(original.size()));

    constexpr std::size_t flg1_payload_offset = 0x23U;
    constexpr std::size_t gala_payload_offset = 0x3CCU;
    original[flg1_payload_offset] ^= 0x55U;
    original[gala_payload_offset] ^= 0x33U;

    UserFile loaded{};
    loaded.loadFromGameDataBinary("mario1", original.data(), static_cast< u32 >(original.size()));

    std::vector< u8 > serialized(SaveDataHandler::getEnoughtTempBufferSize());
    loaded.makeGameDataBinary(serialized.data(), static_cast< u32 >(serialized.size()));

    $pc_port_require_eq(serialized[flg1_payload_offset], original[flg1_payload_offset]);
    $pc_port_require_eq(serialized[gala_payload_offset], original[gala_payload_offset]);
}

$test("SaveDataHandler creates and reloads a Wii-style GameData.bin slot") {
    smgpc::test::TempDirectory temp{};
    setenv("SMGPC_SAVE_DIR", temp.path().string().c_str(), 1);

    SysConfigFile sys_config{};
    UserFile work_file{};
    SaveDataHandler handler(&sys_config, &work_file);

    work_file.resetAllData();
    work_file.setCreated();
    work_file.updateLastModified();
    handler.initializeUserFileMemory(1, &work_file);
    handler.storeSysConfigFile(&sys_config);
    handler.requestSaveSaveData();
    update_until_done(handler);
    $pc_port_require(handler.getLastResultCode().isSuccess());

    const auto save_path = temp.path() / "data" / "GameData.bin";
    $pc_port_require(std::filesystem::exists(save_path));
    const auto bytes = read_binary(save_path);
    $pc_port_require_eq(read_u32_be(bytes, 4U), 2U);
    $pc_port_require_eq(read_u32_be(bytes, 8U), 19U);
    $pc_port_require_eq(read_u32_be(bytes, 0x0CU), 0xBE00U);
    require_game_data_default_chunks(bytes, 0x140U);

    UserFile reloaded_default{};
    SaveDataHandler reloader(&sys_config, &reloaded_default);
    reloader.requestLoadSaveData();
    update_until_done(reloader);
    $pc_port_require(reloader.getLastResultCode().isSuccess());
    $pc_port_require(reloader.requestVerifyAfterLoadGameDataFile());

    std::vector< u8 > temp_buffer(SaveDataHandler::getEnoughtTempBufferSize());
    UserFile restored_slot{};
    reloader.restoreGameDataFile("config1", temp_buffer.data(), static_cast< u32 >(temp_buffer.size()));
    restored_slot.loadFromConfigDataBinary("config1", temp_buffer.data(), static_cast< u32 >(temp_buffer.size()));
    $pc_port_require(restored_slot.isCreated());

    UserFile untouched_slot{};
    reloader.restoreGameDataFile("config2", temp_buffer.data(), static_cast< u32 >(temp_buffer.size()));
    untouched_slot.loadFromConfigDataBinary("config2", temp_buffer.data(), static_cast< u32 >(temp_buffer.size()));
    $pc_port_require(!untouched_slot.isCreated());
}
