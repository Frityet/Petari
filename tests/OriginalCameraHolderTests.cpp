#include "Game/Camera/Camera.hpp"
#include "Game/Camera/CameraFollow.hpp"
#include "Game/Camera/CameraHolder.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraParamChunkHolder.hpp"
#include "Game/Camera/CameraParamChunkID.hpp"
#include "Game/Camera/CameraSpiral.hpp"
#include "Game/Camera/CameraTower.hpp"
#include "Game/Camera/CameraWaterPlanet.hpp"
#include "Game/Camera/DotCamParams.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace smgpc::compat;
using namespace smgpc::resource;

void require(bool condition, const char* message) {
    if (!condition) {
        JkrHostAllocationScope host;
        throw std::runtime_error(message);
    }
}

struct Owners {
    std::shared_ptr<JkrHeapRuntime> heaps = JkrHeapRuntime::create(16 * 1024 * 1024);
    std::shared_ptr<JkrAllocationDomain> domain = JkrAllocationDomain::create(heaps, 8 * 1024 * 1024);
    NameObjRuntimeRegistrationMarker marker = mark_name_obj_runtime_registrations();
    CameraHolder* cameras = nullptr;
    CameraParamChunkHolder* chunks = nullptr;

    Owners() {
        try {
            JkrAllocationScope game(domain);
            cameras = new CameraHolder("Original controller ownership test");
            chunks = new CameraParamChunkHolder(cameras, "Original parameter ownership test");
        } catch (...) {
            retire();
            throw;
        }
    }

    void retire() noexcept {
        const auto in_domain = [](const NameObj* object, const void* context) noexcept {
            return JKRHeap::findFromRoot(const_cast<NameObj*>(object)) == context;
        };
        while (auto* object = newest_name_obj_runtime_object_since_if(marker, in_domain, &domain->heap())) {
            delete object;
        }
    }

    ~Owners() { retire(); }
};

void put32(std::vector<u8>& bytes, std::size_t at, u32 value) {
    for (unsigned i = 0; i < 4; ++i) bytes[at + i] = static_cast<u8>(value >> (24 - 8 * i));
}

struct Row { const char* id; const char* type; };

std::vector<u8> table_bytes(std::span<const Row> rows, bool include_num1 = true) {
    struct Field { const char* name; BcsvFieldType type; };
    const std::array fields{
        Field{"version", BcsvFieldType::UInt32}, Field{"id", BcsvFieldType::StringOffset},
        Field{"camtype", BcsvFieldType::StringOffset}, Field{"string", BcsvFieldType::StringOffset},
        Field{include_num1 ? "num1" : "unused", BcsvFieldType::Int32}, Field{"num2", BcsvFieldType::Int32},
        Field{"dist", BcsvFieldType::Float}, Field{"axis.X", BcsvFieldType::Float},
        Field{"axis.Y", BcsvFieldType::Float}, Field{"axis.Z", BcsvFieldType::Float},
        Field{"angleA", BcsvFieldType::Float}, Field{"angleB", BcsvFieldType::Float},
        Field{"flag.noreset", BcsvFieldType::Int32}, Field{"flag.collisionoff", BcsvFieldType::Int32},
        Field{"evpriority", BcsvFieldType::Int32}, Field{"evfrm", BcsvFieldType::Int32},
    };
    const auto data_offset = 16 + fields.size() * 12;
    const auto entry_size = fields.size() * 4;
    const auto strings_offset = data_offset + entry_size * rows.size();
    std::vector<u8> bytes(strings_offset);
    put32(bytes, 0, rows.size()); put32(bytes, 4, fields.size());
    put32(bytes, 8, data_offset); put32(bytes, 12, entry_size);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto at = 16 + i * 12;
        put32(bytes, at, jmap_hash(fields[i].name)); put32(bytes, at + 4, 0xffffffff);
        bytes[at + 8] = static_cast<u8>((i * 4) >> 8); bytes[at + 9] = static_cast<u8>(i * 4);
        bytes[at + 11] = static_cast<u8>(fields[i].type);
    }
    const auto add_string = [&](const char* text) {
        const u32 offset = bytes.size() - strings_offset;
        bytes.insert(bytes.end(), text, text + std::strlen(text) + 1);
        return offset;
    };
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto at = data_offset + entry_size * i;
        put32(bytes, at, 0x30016);
        for (const auto& [offset, text] : std::array{
                 std::pair{4, rows[i].id}, std::pair{8, rows[i].type}, std::pair{12, "retained original parameter"}}) {
            const auto value = add_string(text);
            put32(bytes, at + offset, value);
        }
        put32(bytes, at + 16, static_cast<u32>(-3)); put32(bytes, at + 20, 0x12345678);
        for (const auto& [offset, value] : std::array{
                 std::pair{24, 42.F}, std::pair{28, 1500.F}, std::pair{32, 300.F},
                 std::pair{36, 0.F}, std::pair{40, 0.5F}, std::pair{44, 0.75F}})
            put32(bytes, at + offset, std::bit_cast<u32>(value));
        put32(bytes, at + 48, 1); put32(bytes, at + 52, 1);
        put32(bytes, at + 56, 3); put32(bytes, at + 60, 120);
    }
    return bytes;
}

CameraParamChunk* register_id(Owners& owners, s32 zone, const char* name) {
    CameraParamChunkID id;
    id.mZoneID = zone;
    id.mName = const_cast<char*>(name);
    JkrAllocationScope game(owners.domain);
    return owners.chunks->createChunk(id, &owners.domain->heap());
}

void test_complete_controller_table() {
    const auto before = snapshot_name_obj_runtime_objects().size();
    {
        Owners owners;
        require(owners.cameras->getNum() == 45, "original holder constructs all 45 controllers");
        for (s32 i = 0; i < owners.cameras->getNum(); ++i) {
            auto* camera = owners.cameras->getCameraInner(i);
            auto* translator = owners.cameras->getTranslator(i);
            require(camera && translator && translator->getCamera() == camera,
                    "every original virtual translator returns its actual constructed controller");
            require(owners.cameras->getIndexOf(camera) == i &&
                    owners.cameras->getIndexOf(owners.cameras->getNameStrOf(i)) == i,
                    "original name and pointer lookups retain complete table order");
            require(JKRHeap::findFromRoot(camera) == &owners.domain->heap() &&
                    JKRHeap::findFromRoot(translator) == &owners.domain->heap(),
                    "all controllers and translators reside in the actual Game arena");
        }
        require(owners.cameras->getIndexOfDefault() == 0 &&
                owners.cameras->getDefaultCamera() == owners.cameras->getCameraInner(0) &&
                std::string_view(owners.cameras->getNameStrOfDefault()) == "CAM_TYPE_XZ_PARA",
                "original default is the first parallel-camera instance");
        require(owners.cameras->getIndexOf("not-a-retail-camera") == -1,
                "unknown type keeps the original lookup failure");
        require(!owners.cameras->isPublic(owners.cameras->getIndexOf("CAM_TYPE_ANIM")) &&
                !owners.cameras->isPublic(owners.cameras->getIndexOf("CAM_TYPE_BLACK_HOLE")) &&
                owners.cameras->isPublic(owners.cameras->getIndexOf("CAM_TYPE_SUBJECTIVE")),
                "private and subjective entries keep original table flags");
    }
    require(snapshot_name_obj_runtime_objects().size() == before,
            "typed camera and height-arranger teardown retires host registrations before arena release");
}

void test_chunk_identity_sort_and_capacity() {
    Owners owners;
    auto* event = register_id(owners, 2, "e:Demo");
    auto* game = register_id(owners, 0, "s:0001");
    auto* other = register_id(owners, 0, "o:Default");
    require(std::string_view(event->getClassName()) == "Event" &&
            std::string_view(game->getClassName()) == "Game" &&
            std::string_view(other->getClassName()) == "Base", "original ID prefix selects the actual chunk subclass");
    require(register_id(owners, 2, "e:Demo") == nullptr && owners.chunks->mNrChunks == 3,
            "duplicate ID registration retains the first chunk without consuming capacity");
    owners.chunks->sort();
    require(owners.chunks->mChunks[0] == event && owners.chunks->mChunks[1] == game &&
            owners.chunks->mChunks[2] == other, "original selection sort preserves descending zone/name order");
    CameraParamChunkID id;
    char event_name[] = "e:Demo";
    id.mZoneID = 2; id.mName = event_name;
    require(owners.chunks->getChunk(id) == event && owners.chunks->findChunk(0, "s:0001") == game,
            "sorted and explicit zone/name lookups retain original chunk identity");
    id.mName = nullptr;
    require(owners.chunks->getChunk(id) == nullptr, "absent chunk ID retains original null behavior");
    // Retail capacity is fixed; exercise every valid slot without inventing an overflow policy.
    for (u32 i = owners.chunks->mNrChunks; i < owners.chunks->mChunkCapacity; ++i) {
        const auto name = "o:capacity-" + std::to_string(i);
        require(register_id(owners, 0, name.c_str()) != nullptr, "each remaining original capacity slot is usable");
    }
    require(owners.chunks->mNrChunks == 1024, "original holder retains its full 0x400 capacity");
}

void test_binary_load_and_virtual_translators() {
    Owners owners;
    const std::array rows{Row{"s:0001", "CAM_TYPE_FOLLOW"}, Row{"e:Tower", "CAM_TYPE_TOWER"},
                          Row{"o:Water", "CAM_TYPE_WATER_PLANET"}};
    auto bytes = std::make_shared<const std::vector<u8>>(table_bytes(rows));
    auto registration = register_jmap_source(*bytes, bytes);
    std::array<CameraParamChunk*, rows.size()> chunks{};
    {
        DotCamReaderInBin reader(bytes->data());
        for (std::size_t i = 0; i < rows.size(); ++i, reader.nextToChunk()) {
            auto* chunk = chunks[i] = register_id(owners, 0, rows[i].id);
            JkrAllocationScope game(owners.domain);
            chunk->load(&reader, owners.cameras);
            require(chunk->mGeneralParam->mNum1 == static_cast<intptr_t>(-3) &&
                    chunk->mGeneralParam->mNum2 == 0x12345678,
                    "signed BCSV num1 sign-extends into pointer-width storage without overwriting num2");
            require(chunk->isOnNoReset() && chunk->isCollisionOff(), "original flag schema loads into actual chunk flags");
            owners.cameras->getTranslator(chunk->mCameraTypeIndex)->setParam(chunk);
        }
    }
    for (auto* chunk : chunks)
        require(std::string_view(chunk->mGeneralParam->mString.getCharPtr()) == "retained original parameter",
                "retained resource registration preserves borrowed strings after stack reader destruction");
    auto* follow = dynamic_cast<CameraFollow*>(owners.cameras->getCameraInner(chunks[0]->mCameraTypeIndex));
    auto* tower = dynamic_cast<CameraTower*>(owners.cameras->getCameraInner(chunks[1]->mCameraTypeIndex));
    auto* water = dynamic_cast<CameraWaterPlanet*>(owners.cameras->getCameraInner(chunks[2]->mCameraTypeIndex));
    require(follow && follow->mDistMax == 1500.F && follow->mDistMin == 300.F && follow->mAngleXMin == 0.5F &&
            follow->mAngleXRange == 0.75F && follow->mAngleYRoundSpeed == 42.F && !follow->mCheckWall,
            "actual Follow translator updates original controller fields and signed boolean semantics");
    require(tower && tower->mDist == 42.F && tower->mAngleX == 0.75F &&
            std::abs(tower->mAngleYRoundSpeed - 0.78539816339F) < 0.000001F,
            "actual Tower translator performs the original angular conversion");
    require(water && water->mDistMin == 1500.F && water->mDistMax == 300.F && water->mAngleX == 0.5F,
            "actual water controller translator retains the original distinct parameter mapping");
    auto* spiral = dynamic_cast<CameraSpiral*>(owners.cameras->getCameraInner(
        owners.cameras->getIndexOf("CAM_TYPE_SPIRAL_DEMO")));
    chunks[0]->mGeneralParam->mNum1 = static_cast<s32>(0xfffe0003U);
    owners.cameras->getTranslator(owners.cameras->getIndexOf("CAM_TYPE_SPIRAL_DEMO"))->setParam(chunks[0]);
    require(spiral && spiral->mEndTime == -2 && spiral->mStartTime == 3,
            "actual Spiral translator preserves signed big-endian halfword order on native hosts");
    auto absent_bytes = std::make_shared<const std::vector<u8>>(table_bytes(std::span(rows).first(1), false));
    auto absent_registration = register_jmap_source(*absent_bytes, absent_bytes);
    DotCamReaderInBin absent(absent_bytes->data());
    constexpr intptr_t retained_pointer_bits = static_cast<intptr_t>(0x123456789abcULL);
    chunks[0]->mGeneralParam->mNum1 = retained_pointer_bits;
    chunks[0]->load(&absent, owners.cameras);
    require(chunks[0]->mGeneralParam->mNum1 == retained_pointer_bits,
            "absent BCSV num1 preserves the complete existing pointer-width value");
}

void test_optional_disc_parameters() {
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !*disc) {
        std::cout << "[skip] real archived camera chunks (set SMGPC_REAL_DISC)\n";
        return;
    }
    require(aurora_dvd_open(disc), "camera owner test opens the requested disc");
    struct CloseDisc { ~CloseDisc() { aurora_dvd_close(); } } close;
    DVDInit();
    smgpc::runtime::DvdFileSystemService dvd("/");
    for (const char* stage : {"HeavensDoorGalaxy", "EggStarGalaxy"}) {
        smgpc::scene::StageHolderOccurrence root;
        root.stage_name = stage;
        const std::array holders{root};
        StageResourceBinding resources(dvd, holders, {});
        Owners owners;
        const auto archive = dvd.retain_archive_for_path(std::string("/StageData/") + stage + ".arc");
        const auto bytes = archive->resource_data("CameraParam.bcam");
        const auto binary = BcsvTable::from_bytes(bytes);
        for (std::size_t row = 0; row < binary.entry_count(); ++row) {
            const auto id = binary.get_string(row, "id");
            require(id && register_id(owners, 0, std::string(*id).c_str()),
                    "real camera IDs register uniquely through original chunk ownership");
        }
        {
            JkrAllocationScope game(owners.domain);
            owners.chunks->loadFile(0);
        }
        require(owners.chunks->mCameraVersion == 0x30016,
                "original loadFile retrieves the actual stage camera archive version");
        for (std::size_t row = 0; row < binary.entry_count(); ++row) {
            const auto id = binary.get_string(row, "id");
            auto* chunk = owners.chunks->findChunk(0, std::string(*id).c_str());
            const auto expected = binary.get_string(row, "camtype");
            require(expected && chunk->mCameraTypeIndex == owners.cameras->getIndexOf(std::string(*expected).c_str()),
                    "real camera type resolves through the complete original table");
            if (const auto number = binary.get_s32(row, "num1"))
                require(chunk->mGeneralParam->mNum1 == *number, "actual archived num1 retains signed original value");
        }
        owners.chunks->sort();
        require(owners.chunks->mNrChunks == binary.entry_count(),
                "actual holder retains every authored row from the complete stage camera resource");
        std::cout << "[retail] " << stage << ": " << binary.entry_count() << " original chunks\n";
    }
}
} // namespace

int main() {
    try {
        for (const auto& [name, test] : std::array{
                 std::pair{"complete original controller table and teardown", test_complete_controller_table},
                 std::pair{"original chunk identity, sort and capacity", test_chunk_identity_sort_and_capacity},
                 std::pair{"binary parameters and actual virtual translators", test_binary_load_and_virtual_translators},
                 std::pair{"real archived original chunks", test_optional_disc_parameters}}) {
            test();
            std::cout << "[pass] " << name << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Original camera holder test failed: " << error.what() << '\n';
        return 1;
    }
}
