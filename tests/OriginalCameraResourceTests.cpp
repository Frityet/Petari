#include "Game/Camera/CameraParamChunkID.hpp"
#include "Game/Camera/CameraParamString.hpp"
#include "Game/Camera/DotCamParams.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
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

void put32(std::vector<u8>& bytes, std::size_t offset, u32 value) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<u8>(value >> (24 - i * 8));
}

struct Field {
    const char* name;
    u16 offset;
    BcsvFieldType type;
    u32 mask = 0xffffffff;
    u8 shift = 0;
};

constexpr std::array fields{
    Field{"version", 0, BcsvFieldType::UInt32},
    Field{"num1", 4, BcsvFieldType::Int32},
    Field{"short", 8, BcsvFieldType::Int16},
    Field{"byte", 10, BcsvFieldType::Int8},
    Field{"packed", 12, BcsvFieldType::UInt32, 0xf00, 8},
    Field{"dist", 16, BcsvFieldType::Float},
    Field{"axis.X", 20, BcsvFieldType::Float},
    Field{"axis.Y", 24, BcsvFieldType::Float},
    Field{"axis.Z", 28, BcsvFieldType::Float},
    Field{"id", 32, BcsvFieldType::StringOffset},
    Field{"camtype", 36, BcsvFieldType::InlineString},
    Field{"partial.X", 68, BcsvFieldType::Float},
    Field{"partial.Y", 72, BcsvFieldType::Float},
};

std::vector<u8> fixture(u32 rows = 2) {
    constexpr u32 data_offset = 16 + fields.size() * 12;
    constexpr u32 entry_size = 76;
    constexpr char names[] = "s:004e\0e:Demo\0";
    std::vector<u8> bytes(data_offset + rows * entry_size + sizeof(names), 0);
    put32(bytes, 0, rows);
    put32(bytes, 4, fields.size());
    put32(bytes, 8, data_offset);
    put32(bytes, 12, entry_size);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        const auto at = 16 + i * 12;
        put32(bytes, at, jmap_hash(field.name));
        put32(bytes, at + 4, field.mask);
        bytes[at + 8] = static_cast<u8>(field.offset >> 8);
        bytes[at + 9] = static_cast<u8>(field.offset);
        bytes[at + 10] = field.shift;
        bytes[at + 11] = static_cast<u8>(field.type);
    }
    for (u32 row = 0; row < rows; ++row) {
        const auto at = data_offset + row * entry_size;
        put32(bytes, at, row == 0 ? 0x30016 : 0x30015);
        put32(bytes, at + 4, static_cast<u32>(row == 0 ? -3 : 1234567));
        bytes[at + 8] = 0x83; bytes[at + 9] = 0;
        bytes[at + 10] = 0xf9;
        put32(bytes, at + 12, 0xfffffaff);
        put32(bytes, at + 16, std::bit_cast<u32>(row == 0 ? 25.5F : -12.25F));
        for (u32 axis = 0; axis < 3; ++axis)
            put32(bytes, at + 20 + axis * 4, std::bit_cast<u32>(float(row * 10 + axis + 1)));
        put32(bytes, at + 32, row == 0 ? 0 : 7);
        constexpr char type[] = "CAM_TYPE_XZ_PARA";
        std::memcpy(bytes.data() + at + 36, type, sizeof(type));
        put32(bytes, at + 68, std::bit_cast<u32>(91.F));
        put32(bytes, at + 72, std::bit_cast<u32>(92.F));
    }
    std::memcpy(bytes.data() + data_offset + rows * entry_size, names, sizeof(names));
    return bytes;
}

void test_borrowed_parameter_strings() {
    CameraParamString first, second;
    require(first.getCharPtr() == nullptr, "parameter string starts absent");
    char text[] = "Authored camera parameter";
    first.setCharPtr(text);
    second = first;
    require(first.getCharPtr() == text && second.getCharPtr() == text,
            "assignment preserves borrowed character identity rather than allocating a copy");
    text[0] = 'a';
    require(second.getCharPtr()[0] == 'a', "borrowed parameter observes its retained source");
    first.setCharPtr("");
    require(first.getCharPtr() == nullptr && second.getCharPtr() == text,
            "empty input normalizes only the destination to absent");
    second.copy(nullptr);
    require(second.getCharPtr() == nullptr, "null input remains absent");
    first = first;
    require(first.getCharPtr() == nullptr, "self assignment preserves absence");
}

void test_id_format_and_temporary_storage() {
    CameraParamChunkID_Tmp id;
    id.createCubeID(255, 0xabcd);
    require(id.mName == id.mBuffer && id.mZoneID == -1 && id.equals(-1, "c:abcd"),
            "cube ID uses original temporary buffer, four hex digits and signed zone narrowing");
    id.createStartID(128, 0x4e);
    require(id.mName == id.mBuffer && id.equals(-128, "s:004e"), "start ID preserves signed zone identity");
    id.createGroupID(7, "Mario", 2, 31);
    require(id.equals(7, "g:Mario:2:31"), "group ID preserves original decimal argument formatting");
    id.createOtherID(2, "Default");
    require(id.equals(2, "o:Default"), "other ID retains the original prefix");
    id.createEventID(3, "Demo");
    require(id.equals(3, "e:Demo"), "event ID retains the original prefix");
    const std::string long_name(300, 'x');
    id.createEventID(0, long_name.c_str());
    require(id.mName == id.mBuffer && std::strlen(id.mName) == 255 && id.mName[255] == 0,
            "original 256-byte formatting buffer bounds long authored names");
}

void test_id_order_copy_and_scene_allocation() {
    auto runtime = JkrHeapRuntime::create(1024 * 1024);
    auto domain = JkrAllocationDomain::create(runtime, 256 * 1024);
    JkrAllocationScope game(domain);
    CameraParamChunkID unnamed, other_unnamed;
    unnamed.mZoneID = -20;
    other_unnamed.mZoneID = 50;
    CameraParamChunkID_Tmp first, later, next_zone;
    first.createEventID(-1, "a");
    later.createEventID(-1, "b");
    next_zone.createEventID(0, "a");
    require(unnamed == other_unnamed && !(unnamed > other_unnamed),
            "two unnamed IDs compare equal regardless of zone");
    require(unnamed > first && !(first > unnamed) && !(unnamed == first),
            "the original unnamed sentinel sorts after every named ID");
    require(later > first && next_zone > later && !(first > first) && first == first,
            "named IDs sort by signed zone before lexicographic name");
    require(!first.equals(0, "e:a") && !first.equals(-1, "e:A"),
            "zone and case remain part of named ID identity");
    CameraParamChunkID copy(first);
    require(copy == first && copy.mName != first.mName &&
                JKRHeap::findFromRoot(copy.mName) == &domain->heap(),
            "persistent copy owns distinct string bytes in the active original scene heap");
    first.createEventID(5, "replacement");
    require(copy.equals(-1, "e:a"), "persistent ID remains stable when the temporary buffer is reused");
    CameraParamChunkID persistent;
    persistent.createOtherID(9, "Default");
    require(JKRHeap::findFromRoot(persistent.mName) == &domain->heap(),
            "original ID factory allocations use the selected scene heap");
    // These original IDs have no owning-string destructor: the scene arena
    // owns the copied name allocations, exactly as it does for camera chunks.
}

void test_binary_fields_and_atomic_vector() {
    auto bytes = std::make_shared<std::vector<u8>>(fixture());
    bytes->insert(bytes->begin(), 0x5a);
    const auto unaligned = std::span<const u8>(*bytes).subspan(1);
    auto registration = register_jmap_source(unaligned, bytes);
    DotCamReaderInBin reader(unaligned.data());
    require(reader.getVersion() == 0x30016 && reader.hasMoreChunk(),
            "original reader attaches an unaligned bounded big-endian resource and reads its first version");
    s32 value = 0;
    require(reader.getValueInt("num1", &value) && value == -3, "signed 32-bit camera parameter decodes exactly");
    require(reader.getValueInt("short", &value) && value == -32000, "signed short camera field sign extends");
    require(reader.getValueInt("byte", &value) && value == -7, "signed byte camera field sign extends");
    require(reader.getValueInt("packed", &value) && value == 10, "packed integer applies authored mask and shift");
    f32 distance = 0;
    require(reader.getValueFloat("dist", &distance) && distance == 25.5F, "camera float decodes big-endian bytes");
    TVec3f axis(0.F, 0.F, 0.F);
    require(reader.getValueVec("axis", &axis) && axis.x == 1 && axis.y == 2 && axis.z == 3,
            "vector getter combines exactly the .X .Y .Z fields");
    TVec3f unchanged(7.F, 8.F, 9.F);
    require(!reader.getValueVec("partial", &unchanged) && unchanged.x == 7 && unchanged.y == 8 && unchanged.z == 9,
            "missing final vector component leaves all caller components unchanged");
    value = 123;
    require(!reader.getValueInt("missing", &value) && value == 123, "absent integer preserves the caller value");
    const char* name = nullptr;
    require(reader.getValueString("id", &name) && std::string_view(name) == "s:004e", "offset string reads original camera ID");
    const char* again = nullptr;
    require(reader.getValueString("id", &again) && again == name, "repeated original reads preserve borrowed string identity");
    require(reader.getValueString("camtype", &name) && std::string_view(name) == "CAM_TYPE_XZ_PARA",
            "inline string reads authored controller type without reducing the catalog");
    reader.nextToChunk();
    require(reader.hasMoreChunk() && reader.getVersion() == 0x30016, "iteration advances the row without replacing table version");
    require(reader.getValueInt("num1", &value) && value == 1234567 &&
                reader.getValueString("id", &name) && std::string_view(name) == "e:Demo",
            "the next chunk retains original row order and values");
    reader.nextToChunk();
    require(!reader.hasMoreChunk() && reader.mMapIter.mIndex == 2, "reader reaches the original end sentinel");
    reader.nextToChunk();
    require(reader.mMapIter.mIndex == 2 && !reader.getValueInt("num1", &value),
            "advancing an invalid end iterator neither runs away nor exposes another row");
    JMapResource empty(fixture(0));
    DotCamReaderInBin empty_reader(empty.data());
    require(empty_reader.getVersion() == 0 && !empty_reader.hasMoreChunk(), "empty authored table has no version row or chunks");
}

void test_reader_resource_and_heap_lifetime() {
    auto runtime = JkrHeapRuntime::create(1024 * 1024);
    auto domain = JkrAllocationDomain::create(runtime, 256 * 1024);
    auto bytes = std::make_shared<const std::vector<u8>>(fixture());
    std::weak_ptr<const std::vector<u8>> source = bytes;
    std::optional<JMapSourceRegistration> registration;
    registration.emplace(register_jmap_source(*bytes, bytes));
    DotCamReaderInBin* reader;
    CameraParamString borrowed;
    std::weak_ptr<JMapInfo::DataCompat> data;
    {
        JkrAllocationScope game(domain);
        reader = new DotCamReaderInBin(bytes->data());
        require(JKRHeap::findFromRoot(reader) == &domain->heap(), "actual DotCam reader belongs to the original scene heap");
        const char* name = nullptr;
        require(reader->getValueString("id", &name), "original heap reader reads its retained camera name");
        borrowed.setCharPtr(name);
        data = reader->mMapInfo.mData;
        require(JKRHeap::findFromRoot(const_cast<char*>(name)) == nullptr,
                "native decoded string cache escapes Game allocation routing");
    }
    registration.reset();
    bytes.reset();
    require(!source.expired() && !data.expired() && std::string_view(borrowed.getCharPtr()) == "s:004e",
            "attached original reader retains archive bytes and borrowed parameters after source unpublication");
    delete reader;
    borrowed.setCharPtr(nullptr);
    require(source.expired() && data.expired(), "typed original reader destruction releases the last native resource lease");
    require(domain->heap().mDisposerList.getNumLinks() == 0,
            "embedded native JMap disposer retires before scene arena destruction");
}

void test_optional_disc_catalogs() {
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !*disc) {
        std::cout << "[skip] original real-disc DotCam catalogs (set SMGPC_REAL_DISC)\n";
        return;
    }
    require(aurora_dvd_open(disc), "real-disc camera test opens the requested image");
    struct CloseDisc { ~CloseDisc() { aurora_dvd_close(); } } close;
    DVDInit();
    smgpc::runtime::DvdFileSystemService dvd("/");
    for (const char* stage : {"HeavensDoorGalaxy", "EggStarGalaxy"}) {
        const auto path = std::string("/StageData/") + stage + ".arc";
        const auto owner = dvd.retain_archive_for_path(path);
        const auto bytes = owner->resource_data("CameraParam.bcam");
        const auto table = BcsvTable::from_bytes(bytes);
        auto registered = register_jmap_source(bytes, owner);
        DotCamReaderInBin reader(bytes.data());
        require(table.entry_count() != 0 && reader.getVersion() == table.get_u32(0, "version"),
                "original reader consumes the actual archived CameraParam version");
        std::size_t row = 0, values = 0, vectors = 0;
        for (; reader.hasMoreChunk(); reader.nextToChunk(), ++row) {
            require(row < table.entry_count(), "original iteration stays within actual archived row count");
            // Compare the original reader's camera schema to the bounded
            // binary decoder; synthetic cases independently fix byte values.
            for (const char* key : {"version", "num1", "num2", "camint", "gflag", "vpanuse", "evpriority", "evfrm", "uplay"}) {
                if (const auto expected = table.get_s32(row, key)) {
                    s32 actual = 0;
                    require(reader.getValueInt(key, &actual) && actual == *expected, "retail camera integer matches bounded binary table");
                    ++values;
                }
            }
            for (const char* key : {"dist", "angleA", "angleB", "fovy", "loffset", "loffsetv", "roll", "camendint", "upper", "lower", "gndint", "uplaydist", "lplay", "pushdelay", "pushdelaylow"}) {
                if (const auto expected = table.get_float(row, key)) {
                    f32 actual = 0;
                    require(reader.getValueFloat(key, &actual) && std::bit_cast<u32>(actual) == std::bit_cast<u32>(*expected),
                            "retail camera float preserves exact binary value");
                    ++values;
                }
            }
            for (const char* key : {"id", "camtype", "string"}) {
                if (const auto expected = table.get_string(row, key)) {
                    const char* actual = nullptr;
                    require(reader.getValueString(key, &actual) && actual && actual == *expected,
                            "retail camera string matches actual archived bytes");
                    ++values;
                }
            }
            for (const char* key : {"axis", "wpoint", "up", "woffset", "vpanaxis"}) {
                const std::string prefix(key);
                const auto x = table.get_float(row, prefix + ".X");
                const auto y = table.get_float(row, prefix + ".Y");
                const auto z = table.get_float(row, prefix + ".Z");
                if (x && y && z) {
                    TVec3f actual;
                    require(reader.getValueVec(key, &actual) && actual.x == *x && actual.y == *y && actual.z == *z,
                            "retail vector getter preserves complete authored vector");
                    ++vectors;
                }
            }
        }
        require(row == table.entry_count() && values > row && vectors != 0,
                "original reader traverses every authored row and present camera values");
        std::cout << "[retail] " << stage << ": " << row << " rows, " << values << " scalar/string fields, "
                  << vectors << " vectors, version " << reader.getVersion() << '\n';
    }
}
} // namespace

int main() {
    try {
        const std::array tests{
            std::pair{"borrowed parameter strings", test_borrowed_parameter_strings},
            std::pair{"original ID formatting/temporary storage", test_id_format_and_temporary_storage},
            std::pair{"ID order/copy/scene ownership", test_id_order_copy_and_scene_allocation},
            std::pair{"original binary fields/vector/iteration", test_binary_fields_and_atomic_vector},
            std::pair{"original reader resource/heap lifetime", test_reader_resource_and_heap_lifetime},
            std::pair{"actual disc camera catalogs", test_optional_disc_catalogs},
        };
        for (const auto& [name, test] : tests) {
            test();
            std::cout << "[pass] " << name << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Original camera resource tests failed: " << error.what() << '\n';
        return 1;
    }
}
