#include "J3dMaterialTableData.hpp"
#include "J3dAllocationIdentity.hpp"
#include "J3dMaterialBlockData.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DMaterialFactory.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace smgpc::resource {
    namespace {
        using Bytes = std::span<const std::uint8_t>;
        using MaterialType = J3DMaterialFactory::MaterialType;
        constexpr auto normal = J3DMaterialFactory::MATERIAL_TYPE_NORMAL;
        constexpr auto patched = J3DMaterialFactory::MATERIAL_TYPE_PATCHED;
        constexpr auto locked = J3DMaterialFactory::MATERIAL_TYPE_LOCKED;
        constexpr std::size_t original_material_stride = 0x4C;

        template <typename Error = std::runtime_error>
        [[noreturn]] void fail(const char* message) {
            aurora::allocation::HostAllocationScope host;
            throw Error(message);
        }

        void require_range(Bytes bytes, std::size_t offset, std::size_t size) {
            if (offset > bytes.size() || size > bytes.size() - offset)
                fail("J3D material construction exceeds its containing file");
        }
        std::uint32_t u32_at(Bytes bytes, std::size_t offset) {
            require_range(bytes, offset, 4);
            return std::uint32_t(bytes[offset]) << 24 | std::uint32_t(bytes[offset + 1]) << 16 |
                   std::uint32_t(bytes[offset + 2]) << 8 | bytes[offset + 3];
        }

        struct MaterialAllocation {
            J3DMaterial* value;
            MaterialType type;
            MaterialAllocation(J3DMaterial* pointer, MaterialType kind) : value(pointer), type(kind) {}
            MaterialAllocation(MaterialAllocation&& other) noexcept : value(std::exchange(other.value, nullptr)), type(other.type) {}
            MaterialAllocation(const MaterialAllocation&) = delete;
            ~MaterialAllocation() {
                if (value == nullptr) return;
                // Material has no virtual destructor. Destroy the actual class
                // constructed by the original factory. Its subsidiary objects
                // and later original allocations remain owned by the SDK heap.
                if (type == patched) std::destroy_at(static_cast<J3DPatchedMaterial*>(value));
                else if (type == locked) std::destroy_at(static_cast<J3DLockedMaterial*>(value));
                else std::destroy_at(value);
                ::operator delete(value);
            }
        };
    }

    struct J3dMaterialTableData::Storage {
        // SDK values die before native backing and the final retained heap.
        JKRHeap::Handle domain;
        std::vector<std::unique_ptr<J3dMaterialBlockData>> blocks;
        std::vector<std::unique_ptr<J3dAllocationIdentity>> identities;
        std::vector<std::unique_ptr<JUTNameTab>> names;
        std::vector<std::unique_ptr<J3DMaterial*[]>> pointer_arrays;
        std::vector<std::unique_ptr<J3DMaterial[]>> unique_arrays;
        std::vector<MaterialAllocation> materials;
        std::vector<std::uint32_t> tex_no_offsets;
        J3DMaterialTable table;
        bool attached = false;

        Storage(Bytes bytes, std::uint32_t flags, Mode mode, JKRHeap::Handle owner)
            : domain(std::move(owner)) {
            if (!domain) fail<std::invalid_argument>("Original material construction requires a retained JKR heap");
            require_range(bytes, 0, 0x20);
            const auto format = u32_at(bytes, 4);
            if (u32_at(bytes, 0) != 0x4A334432U ||
                (format != 0x626D6433U && format != 0x62646C33U && format != 0x62646C34U && format != 0x626D7433U)) {
                fail("Original v26 material construction requires J3D2 BMD3/BDL3/BDL4/BMT3 metadata");
            }
            const auto size = u32_at(bytes, 8);
            if (size < 0x20) fail("J3D material file header is truncated");
            require_range(bytes, 0, size);
            bytes = bytes.first(size);
            const auto count = u32_at(bytes, 0xC);
            J3DModelLoader_v26 loader;
            loader.mpMaterialTable = &table;
            std::size_t cursor = 0x20;
            for (std::uint32_t i = 0; i < count; ++i) {
                require_range(bytes, cursor, 8);
                const auto type = u32_at(bytes, cursor);
                const auto block_size = u32_at(bytes, cursor + 4);
                if (block_size < 8) fail("J3D material block header is truncated");
                require_range(bytes, cursor, block_size);
                if (type == 0x4D415432U && mode != Mode::BinaryModel)
                    fail("MAT2 requires the original v21 material loader");
                if (type == 0x4D415433U || (type == 0x4D444C33U && mode == Mode::BinaryModel)) {
                    auto block = std::make_unique<J3dMaterialBlockData>(bytes.subspan(cursor, block_size));
                    const auto* retained = block.get();
                    blocks.push_back(std::move(block));
                    JKRHeap::CurrentHeapScope original_allocations(*(domain));
                    const aurora::allocation::ClientAllocationScope original_allocations_routing({true, true});
                    if (type == 0x4D415433U) {
                        if (mode == Mode::MaterialTable) {
                            read_material(loader, retained->material(), 0x51100000U, normal, true);
                        } else if (mode == Mode::Model) {
                            read_material(loader, retained->material(), flags, normal, false);
                        } else {
                            const auto material_flags = 0x50100000U | (flags & 0x03000000U);
                            loader.mpMaterialBlock = &retained->material();
                            if ((flags & 0x3000U) == 0) read_material(loader, *loader.mpMaterialBlock, material_flags, normal, false);
                            else if ((flags & 0x3000U) == 0x2000U) read_material(loader, *loader.mpMaterialBlock, material_flags, patched, false);
                        }
                    } else {
                        read_material_dl(loader, retained->display_list(), flags);
                    }
                }
                cursor += block_size;
            }
        }

        // The original loader owns construction policy. Keep its published
        // allocations alive until the decoded resource and native heap retire.
        void retain_created_materials(MaterialType type) {
            aurora::allocation::HostAllocationScope host;
            names.emplace_back(table.mMaterialName);
            pointer_arrays.emplace_back(table.mMaterialNodePointer);
            unique_arrays.emplace_back(table.field_0x10);
            tex_no_offsets.assign(table.mMaterialNum, 0);
            for (u16 i = 0; i < table.mMaterialNum; ++i)
                materials.emplace_back(table.mMaterialNodePointer[i], type);
        }

        J3dAllocationIdentity& identity(const J3DMaterialFactory& factory, std::uint16_t count,
                                       std::uint16_t unique_count, bool unique) {
            std::size_t maximum_id = 0;
            for (u16 i = 0; i < count; ++i) maximum_id = std::max<std::size_t>(maximum_id, factory.getMaterialID(i));
            if (unique && count != 0 && maximum_id >= unique_count)
                fail("MAT3 unique material remap exceeds the original counted array");
            const auto extent = unique ? std::max<std::size_t>(1, unique_count * original_material_stride)
                                       : std::max<std::size_t>(16 * (maximum_id + 1), count * 4U);
            aurora::allocation::HostAllocationScope host;
            auto owner = std::make_unique<J3dAllocationIdentity>(extent);
            auto* result = owner.get();
            identities.push_back(std::move(owner));
            return *result;
        }

        void read_material(J3DModelLoader_v26& loader, const J3DMaterialBlock& block,
                           std::uint32_t flags, MaterialType type, bool table_only) {
            J3DMaterialFactory factory(block);
            const bool unique = !table_only && type == normal && (flags & 0x200000U);
            auto& addresses = identity(factory, block.mMaterialNum, factory.countUniqueMaterials(), unique);
            const J3dAllocationIdentity::Scope address_binding(addresses);
            if (table_only) loader.readMaterialTable(&block, flags);
            else if (type == patched) loader.readPatchedMaterial(&block, flags);
            else loader.readMaterial(&block, flags);
            retain_created_materials(type);
        }

        void read_material_dl(J3DModelLoader_v26& loader, const J3DMaterialDLBlock& block, std::uint32_t flags) {
            if (table.mMaterialNum > block.mMaterialNum)
                fail("MDL3 cannot patch more materials than its authored tables contain");
            if ((flags & 0x2000U) && (loader.mpMaterialBlock == nullptr ||
                    (table.mMaterialNum == 0 ? block.mMaterialNum : table.mMaterialNum) > loader.mpMaterialBlock->mMaterialNum))
                fail("Patched MDL3 requires a preceding MAT3 with matching material indices");
            const bool creates_materials = table.mMaterialNum == 0;
            loader.readMaterialDL(&block, flags);
            if (creates_materials) retain_created_materials(locked);
            loader.modifyMaterial(flags);
            J3DMaterialFactory factory(block);
            for (u16 i = 0; i < table.mMaterialNum; ++i) tex_no_offsets[i] = factory.mpPatchingInfo[i].mTexNoOffset;
        }
    };

    J3dMaterialTableData::J3dMaterialTableData(Bytes bytes, std::uint32_t flags, Mode mode,
                                           JKRHeap::Handle domain) {
        aurora::allocation::HostAllocationScope host;
        _storage = std::make_unique<Storage>(bytes, flags, mode, std::move(domain));
    }
    J3dMaterialTableData::~J3dMaterialTableData() {
        aurora::allocation::HostAllocationScope host;
        _storage.reset();
    }
    void J3dMaterialTableData::attach_to(J3DMaterialTable& target) {
        if (_storage->attached || target.mMaterialNum != 0 || target.mUniqueMatNum != 0 ||
            target.mMaterialNodePointer != nullptr || target.mMaterialName != nullptr ||
            target.field_0x10 != nullptr || target.field_0x1c != 0)
            fail<std::logic_error>("Original material fields can only be attached once to a fresh table");
        const auto& source = _storage->table;
        target.mMaterialNum = source.mMaterialNum;
        target.mUniqueMatNum = source.mUniqueMatNum;
        target.mMaterialNodePointer = source.mMaterialNodePointer;
        target.mMaterialName = source.mMaterialName;
        target.field_0x10 = source.field_0x10;
        target.field_0x1c = source.field_0x1c;
        _storage->attached = true;
    }
    std::uint32_t J3dMaterialTableData::tex_no_patch_offset(std::uint16_t material_index) const {
        if (material_index >= _storage->tex_no_offsets.size()) fail<std::out_of_range>("Material patch offset index exceeds its table");
        return _storage->tex_no_offsets[material_index];
    }
}
