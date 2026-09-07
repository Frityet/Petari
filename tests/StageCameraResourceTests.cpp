#include "compat/StageResourceBinding.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Camera/DotCamParams.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace smgpc::compat;
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
void put32(std::vector<u8>& bytes, std::size_t offset, u32 value) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<u8>(value >> (24 - 8 * i));
}
void put16(std::vector<u8>& bytes, std::size_t offset, u16 value) {
    bytes[offset] = value >> 8; bytes[offset + 1] = value;
}
std::vector<u8> camera_table(const std::string& name) {
    std::vector<u8> bytes(48 + name.size() + 1);
    put32(bytes, 0, 1); put32(bytes, 4, 2); put32(bytes, 8, 40); put32(bytes, 12, 8);
    put32(bytes, 16, smgpc::resource::jmap_hash("version")); put32(bytes, 20, 0xffffffff);
    put32(bytes, 28, smgpc::resource::jmap_hash("id")); put32(bytes, 32, 0xffffffff);
    put16(bytes, 36, 4); bytes[39] = static_cast<u8>(smgpc::resource::BcsvFieldType::StringOffset);
    put32(bytes, 40, 0x30016); put32(bytes, 44, 0);
    std::memcpy(bytes.data() + 48, name.c_str(), name.size() + 1);
    return bytes;
}
    smgpc::resource::RarcArchive make_single_file_rarc(std::string_view file_name,
                                                       const std::vector<std::uint8_t> &file_data) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};
        constexpr auto string_table_offset = std::size_t{0x64U};
        constexpr auto file_data_offset = std::size_t{0x80U};
        require(string_table_offset + file_name.size() + 1U <= file_data_offset,
                "test RARC file name must fit before its data section");

        auto bytes = std::vector<std::uint8_t>(file_data_offset + file_data.size(), 0U);
        put32(bytes, 0x00U, 0x52415243U);
        put32(bytes, 0x04U, static_cast<std::uint32_t>(bytes.size()));
        put32(bytes, 0x08U, header_size);
        put32(bytes, 0x0cU, file_data_offset - header_size);
        put32(bytes, 0x10U, static_cast<std::uint32_t>(file_data.size()));

        put32(bytes, info_offset + 0x00U, 1U);
        put32(bytes, info_offset + 0x04U, directory_offset - info_offset);
        put32(bytes, info_offset + 0x08U, 1U);
        put32(bytes, info_offset + 0x0cU, file_entry_offset - info_offset);
        put32(bytes, info_offset + 0x10U, static_cast<std::uint32_t>(file_name.size() + 1U));
        put32(bytes, info_offset + 0x14U, string_table_offset - info_offset);

        put16(bytes, directory_offset + 0x0aU, 1U);
        put32(bytes, directory_offset + 0x0cU, 0U);

        put16(bytes, file_entry_offset + 0x00U, 0U);
        put16(bytes, file_entry_offset + 0x02U, smgpc::resource::RarcArchive::hash_name(file_name));
        bytes[file_entry_offset + 0x04U] = 1U;
        put32(bytes, file_entry_offset + 0x08U, 0U);
        put32(bytes, file_entry_offset + 0x0cU, static_cast<std::uint32_t>(file_data.size()));

        for (auto index = std::size_t{}; index < file_name.size(); ++index) {
            bytes[string_table_offset + index] = static_cast<std::uint8_t>(file_name[index]);
        }
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + file_data_offset);
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }
struct DiscFixture {
    struct File { std::string path; std::vector<u8> bytes; };
    struct Handle { File* file; std::size_t position = 0; };
    std::vector<std::unique_ptr<File>> files;
    DiscFixture() {
        const AuroraOverlayCallbacks callbacks{
            [](void* data) -> void* {
                JkrHostAllocationScope host;
                return new Handle{static_cast<File*>(data)};
            },
            [](void* data) { delete static_cast<Handle*>(data); },
            [](void* data, u8* output, std::size_t length) -> int64_t {
                auto& handle = *static_cast<Handle*>(data);
                const auto count = std::min(length, handle.file->bytes.size() - handle.position);
                std::memcpy(output, handle.file->bytes.data() + handle.position, count);
                handle.position += count;
                return count;
            },
            [](void* data, int64_t offset, int32_t whence) -> int64_t {
                auto& handle = *static_cast<Handle*>(data);
                const int64_t base = whence == 0 ? 0 : whence == 1 ? handle.position : handle.file->bytes.size();
                if (whence < 0 || whence > 2 || offset < -base || base + offset > handle.file->bytes.size()) return -1;
                handle.position = base + offset;
                return handle.position;
            }
        };
        aurora_dvd_overlay_callbacks(&callbacks);
        DVDInit();
    }
    ~DiscFixture() { aurora_dvd_overlay_files(nullptr, 0, nullptr); }
    void write(const char* stage, const char* id, const char* resource = "CameraParam.bcam") {
        const auto archive = make_single_file_rarc(resource, camera_table(id));
        files.push_back(std::make_unique<File>(File{
            std::string("/StageData/") + stage + ".arc", {archive.bytes().begin(), archive.bytes().end()}}));
        std::vector<AuroraOverlayFile> overlays;
        for (const auto& file : files) overlays.push_back({file->path.c_str(), file.get(), file->bytes.size()});
        aurora_dvd_overlay_files(overlays.data(), overlays.size(), nullptr);
    }
};
smgpc::scene::StageHolderOccurrence holder(std::size_t id, std::optional<std::size_t> parent, s32 zone, const char* name) {
    smgpc::scene::StageHolderOccurrence result;
    result.instance_id = id; result.parent_instance_id = parent; result.zone_id = zone; result.stage_name = name;
    return result;
}
const char* read_name(s32 zone, void** identity = nullptr) {
    void* data = nullptr; s32 size = 0;
    MR::getStageCameraData(&data, &size, zone);
    require(data && size > 0, "placed camera has the bounded original archive identity");
    if (identity) *identity = data;
    DotCamReaderInBin reader(data);
    const char* name = nullptr;
    require(reader.getValueString("id", &name), "original DotCam reader attaches stage-published bytes");
    return name;
}
void test_stage_ownership() {
    DiscFixture fixture;
    fixture.write("Root", "s:root"); fixture.write("First", "s:first");
    fixture.write("Repeated", "s:second"); fixture.write("Nested", "s:nested");
    fixture.write("Empty", "s:unused", "Other.bcsv");
    smgpc::runtime::DvdFileSystemService dvd("/");
    void* bytes = nullptr; s32 size = 0;
    bool rejected = false;
    try { MR::getStageCameraData(&bytes, &size, 0); } catch (const std::logic_error&) { rejected = true; }
    require(rejected, "queries require an actual stage owner");
    auto heaps = JkrHeapRuntime::create(8 * 1024 * 1024);
    auto domain = JkrAllocationDomain::create(heaps, 2 * 1024 * 1024);
    std::optional<StageResourceBinding> binding;
    const char* borrowed;
    std::weak_ptr<const JMapInfo> decoded;
    {
        JkrAllocationScope game(domain);
        auto holders = std::vector{
            holder(0, {}, 0, "Root"), holder(1, 0, 4, "First"),
            holder(2, 0, 4, "Repeated"), holder(3, 1, 9, "Nested"), holder(4, 0, 8, "Empty")};
        holders[0].children = {1, 2, 4}; holders[1].children = {3};
        binding.emplace(dvd, holders);
        borrowed = read_name(4, &bytes);
        decoded = smgpc::resource::find_jmap_resource(bytes);
        require(std::string_view(borrowed) == "s:first", "first matching immediate child retains lookup precedence");
        require(JKRHeap::findFromRoot(const_cast<char*>(borrowed)) == nullptr, "decoded camera strings have host ownership");
        require(std::string_view(read_name(0)) == "s:root", "zero resolves the root archive");
        for (s32 zone : {9, 77, -1}) {
            MR::getStageCameraData(&bytes, &size, zone);
            require(bytes == nullptr && size == 0, "nested-only and unplaced zone IDs return original null/zero result");
        }
        MR::getStageCameraData(&bytes, &size, 8);
        require(bytes == nullptr && size == -1, "placed missing resource preserves original JKRArchive size sentinel");
        require(dvd.archive_load_count("StageData/Repeated.arc") == 0 && dvd.archive_load_count("StageData/Nested.arc") == 0,
                "zone query does not load shadowed or recursively unreachable archives");
    }
    domain.reset();
    auto replacement = JkrAllocationDomain::create(heaps, 2 * 1024 * 1024);
    {
        JkrAllocationScope game(replacement);
        auto* overwrite = new u8[1024 * 1024];
        std::memset(overwrite, 0xa5, 1024 * 1024);
        require(!decoded.expired() && std::string_view(borrowed) == "s:first", "stage registration retains decoded strings after reader and arena retirement");
        require(std::string_view(read_name(4)) == "s:first", "host stage metadata survives retirement and overwrite");
        delete[] overwrite;
    }
    const auto nested_holders = std::array{holder(0, {}, 0, "Repeated")};
    {
        StageResourceBinding nested(dvd, nested_holders);
        require(std::string_view(read_name(0)) == "s:second", "nested stage binding selects its own archive");
    }
    require(std::string_view(read_name(0)) == "s:root", "nested binding teardown restores the prior stage");
    binding.reset();
    require(decoded.expired(), "stage teardown retires the last decoded camera cache registration");
}
void test_real_disc() {
    smgpc::runtime::DvdFileSystemService dvd("/");
    for (const char* stage : {"HeavensDoorGalaxy", "EggStarGalaxy"}) {
        const auto holders = std::array{holder(0, {}, 0, stage)};
        StageResourceBinding binding(dvd, holders);
        void* data = nullptr; s32 size = 0;
        MR::getStageCameraData(&data, &size, 0);
        const auto archive = dvd.retain_archive_for_path(std::string("/StageData/") + stage + ".arc");
        const auto original = archive->resource_data("CameraParam.bcam");
        require(data == original.data() && size == original.size(), "stage camera provider retains actual RVZ archive identity and full bounds");
        const auto reference = smgpc::resource::BcsvTable::from_bytes(original);
        std::vector<const char*> names;
        {
            DotCamReaderInBin reader(data);
            for (; reader.hasMoreChunk(); reader.nextToChunk()) {
                const char* name = nullptr;
                require(reader.getValueString("id", &name), "every original camera row has an ID");
                names.push_back(name);
            }
        }
        require(names.size() == reference.entry_count(), "stage query exposes the complete archived camera catalog");
        for (std::size_t i = 0; i < names.size(); ++i)
            require(std::string_view(names[i]) == reference.get_string(i, "id"), "real catalog strings outlive the temporary original reader");
        std::cout << stage << ": " << names.size() << " retained camera rows\n";
    }
}
}
int main() {
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        if (!disc || !*disc) { std::cout << "[skip] stage camera DVD fixtures (set SMGPC_REAL_DISC)\n"; return 0; }
        require(aurora_dvd_open(disc), "open disc underlying the DVD fixture overlays");
        struct CloseDisc { ~CloseDisc() { aurora_dvd_close(); } } close;
        DVDInit();
        test_stage_ownership(); test_real_disc();
        std::cout << "Stage camera resources passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
