#include "J3dMaterialTableData.hpp"
#include "J3dAllocationIdentity.hpp"
#include "J3dMaterialBlockData.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DMaterialFactory.hpp"
#include "JSystem/JSupport/JSupport.hpp"
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
            compat::JkrHostAllocationScope host;
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
        // SDK values die before native backing and the final retained domain.
        std::shared_ptr<compat::JkrAllocationDomain> domain;
        std::vector<std::unique_ptr<J3dMaterialBlockData>> blocks;
        std::vector<std::unique_ptr<J3dAllocationIdentity>> identities;
        std::vector<std::unique_ptr<JUTNameTab>> names;
        std::vector<std::unique_ptr<J3DMaterial*[]>> pointer_arrays;
        std::vector<std::unique_ptr<J3DMaterial[]>> unique_arrays;
        std::vector<MaterialAllocation> materials;
        std::vector<std::uint32_t> tex_no_offsets;
        J3DMaterialTable table;
        const J3DMaterialBlock* material_block = nullptr;
        bool attached = false;

        Storage(Bytes bytes, std::uint32_t flags, Mode mode, std::shared_ptr<compat::JkrAllocationDomain> owner)
            : domain(std::move(owner)) {
            if (!domain) fail<std::invalid_argument>("Original material construction requires a retained JKR domain");
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
                    compat::JkrAllocationScope original_allocations(domain);
                    if (type == 0x4D415433U) {
                        if (mode == Mode::MaterialTable) {
                            read_material_table(retained->material(), 0x51100000U);
                        } else if (mode == Mode::Model) {
                            read_material(retained->material(), flags);
                        } else {
                            const auto material_flags = 0x50100000U | (flags & 0x03000000U);
                            material_block = &retained->material();
                            if ((flags & 0x3000U) == 0) read_material(*material_block, material_flags);
                            else if ((flags & 0x3000U) == 0x2000U) read_patched_material(*material_block, material_flags);
                        }
                    } else {
                        read_material_dl(retained->display_list(), flags);
                        modify_material(flags);
                    }
                }
                cursor += block_size;
            }
        }

        JUTNameTab* create_name(const void* block, const void* offset) {
            if (offset == nullptr) return nullptr;
            auto name = std::unique_ptr<JUTNameTab>(new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, offset)));
            auto* result = name.get();
            compat::JkrHostAllocationScope host;
            names.push_back(std::move(name));
            return result;
        }
        J3DMaterial** create_pointer_array(std::uint16_t count) {
            auto array = std::unique_ptr<J3DMaterial*[]>(new J3DMaterial*[count]);
            auto* result = array.get();
            compat::JkrHostAllocationScope host;
            pointer_arrays.push_back(std::move(array));
            tex_no_offsets.assign(count, 0);
            return result;
        }
        J3DMaterial* create_unique_array(std::uint16_t count) {
            auto array = std::unique_ptr<J3DMaterial[]>(new (0x20) J3DMaterial[count]);
            auto* result = array.get();
            compat::JkrHostAllocationScope host;
            unique_arrays.push_back(std::move(array));
            return result;
        }
        J3DMaterial* create(const J3DMaterialFactory& factory, J3DMaterial* existing, MaterialType type, int index, std::uint32_t flags) {
            auto* result = factory.create(existing, type, index, flags);
            if (existing == nullptr) {
                MaterialAllocation allocation(result, type);
                compat::JkrHostAllocationScope host;
                materials.push_back(std::move(allocation));
            }
            return result;
        }
        J3dAllocationIdentity& identity(const J3DMaterialFactory& factory, bool unique) {
            std::size_t maximum_id = 0;
            for (u16 i = 0; i < table.mMaterialNum; ++i) maximum_id = std::max<std::size_t>(maximum_id, factory.getMaterialID(i));
            if (unique && table.mMaterialNum != 0 && maximum_id >= table.mUniqueMatNum)
                fail("MAT3 unique material remap exceeds the original counted array");
            const auto extent = unique ? std::max<std::size_t>(1, table.mUniqueMatNum * original_material_stride)
                                       : std::max<std::size_t>(16 * (maximum_id + 1), table.mMaterialNum * 4U);
            compat::JkrHostAllocationScope host;
            auto owner = std::make_unique<J3dAllocationIdentity>(extent);
            auto* result = owner.get();
            identities.push_back(std::move(owner));
            return *result;
        }

        // Original J3DModelLoader_v26::readMaterial, 0x8043EC04. The explicit
        // lifetime helpers above retain original allocations; identity words use
        // disjoint original-width addresses instead of truncating host pointers.
        void read_material(const J3DMaterialBlock& block, std::uint32_t flags) {
            J3DMaterialFactory factory(block);
            table.mMaterialNum = block.mMaterialNum;
            table.mUniqueMatNum = factory.countUniqueMaterials();
            table.mMaterialName = create_name(&block, block.mpNameTable);
            table.mMaterialNodePointer = create_pointer_array(table.mMaterialNum);
            if (flags & 0x200000U) table.field_0x10 = create_unique_array(table.mUniqueMatNum);
            else table.field_0x10 = nullptr;
            auto& addresses = identity(factory, (flags & 0x200000U) != 0);
            if (flags & 0x200000U) {
                for (u16 i = 0; i < table.mUniqueMatNum; ++i) {
                    create(factory, &table.field_0x10[i], normal, i, flags);
                    table.field_0x10[i].mDiffFlag = addresses.address(i * original_material_stride) >> 4;
                }
            }
            for (u16 i = 0; i < table.mMaterialNum; ++i)
                table.mMaterialNodePointer[i] = create(factory, nullptr, normal, i, flags);
            if (flags & 0x200000U) {
                for (u16 i = 0; i < table.mMaterialNum; ++i) {
                    table.mMaterialNodePointer[i]->mDiffFlag = addresses.address(factory.getMaterialID(i) * original_material_stride) >> 4;
                    table.mMaterialNodePointer[i]->mpOrigMaterial = &table.field_0x10[factory.getMaterialID(i)];
                }
            } else {
                for (u16 i = 0; i < table.mMaterialNum; ++i)
                    table.mMaterialNodePointer[i]->mDiffFlag = (addresses.address() >> 4) + factory.getMaterialID(i);
            }
        }

        // Original J3DModelLoader_v26::readMaterialTable, 0x8043F2CC.
        void read_material_table(const J3DMaterialBlock& block, std::uint32_t flags) {
            J3DMaterialFactory factory(block);
            table.mMaterialNum = block.mMaterialNum;
            table.mMaterialName = create_name(&block, block.mpNameTable);
            table.mMaterialNodePointer = create_pointer_array(table.mMaterialNum);
            for (u16 i = 0; i < table.mMaterialNum; ++i)
                table.mMaterialNodePointer[i] = create(factory, nullptr, normal, i, flags);
            auto& addresses = identity(factory, false);
            for (u16 i = 0; i < table.mMaterialNum; ++i)
                table.mMaterialNodePointer[i]->mDiffFlag = addresses.address() + factory.getMaterialID(i);
        }

        // Original J3DModelLoader::readPatchedMaterial, 0x8043F608.
        void read_patched_material(const J3DMaterialBlock& block, std::uint32_t flags) {
            J3DMaterialFactory factory(block);
            table.mMaterialNum = block.mMaterialNum;
            table.mUniqueMatNum = factory.countUniqueMaterials();
            table.mMaterialName = create_name(&block, block.mpNameTable);
            table.mMaterialNodePointer = create_pointer_array(table.mMaterialNum);
            table.field_0x10 = nullptr;
            auto& addresses = identity(factory, false);
            for (u16 i = 0; i < table.mMaterialNum; ++i) {
                table.mMaterialNodePointer[i] = create(factory, nullptr, patched, i, flags);
                table.mMaterialNodePointer[i]->mDiffFlag = (addresses.address() >> 4) + factory.getMaterialID(i);
            }
        }

        // Original J3DModelLoader::readMaterialDL, 0x8043F744.
        void read_material_dl(const J3DMaterialDLBlock& block, std::uint32_t flags) {
            J3DMaterialFactory factory(block);
            if (table.mMaterialNum > block.mMaterialNum)
                fail("MDL3 cannot patch more materials than its authored tables contain");
            if (table.mMaterialNum == 0) {
                table.field_0x1c = 1;
                table.mMaterialNum = block.mMaterialNum;
                table.mUniqueMatNum = block.mMaterialNum;
                table.mMaterialName = create_name(&block, block.mpNameTable);
                table.mMaterialNodePointer = create_pointer_array(table.mMaterialNum);
                table.field_0x10 = nullptr;
                for (u16 i = 0; i < table.mMaterialNum; ++i)
                    table.mMaterialNodePointer[i] = create(factory, nullptr, locked, i, flags);
                for (u16 i = 0; i < table.mMaterialNum; ++i)
                    table.mMaterialNodePointer[i]->mDiffFlag = 0xC0000000U;
            } else {
                for (u16 i = 0; i < table.mMaterialNum; ++i)
                    table.mMaterialNodePointer[i] = create(factory, table.mMaterialNodePointer[i], locked, i, flags);
            }
            for (u16 i = 0; i < table.mMaterialNum; ++i) tex_no_offsets[i] = factory.mpPatchingInfo[i].mTexNoOffset;
        }

        // Original J3DModelLoader::modifyMaterial, 0x8043F8F0.
        void modify_material(std::uint32_t flags) {
            if (flags & 0x2000U) {
                if (material_block == nullptr || table.mMaterialNum > material_block->mMaterialNum)
                    fail("Patched MDL3 requires a preceding MAT3 with matching material indices");
                J3DMaterialFactory factory(*material_block);
                for (u16 i = 0; i < table.mMaterialNum; ++i)
                    factory.modifyPatchedCurrentMtx(table.mMaterialNodePointer[i], i);
            }
        }
    };

    J3dMaterialTableData::J3dMaterialTableData(Bytes bytes, std::uint32_t flags, Mode mode,
                                           std::shared_ptr<compat::JkrAllocationDomain> domain) {
        compat::JkrHostAllocationScope host;
        _storage = std::make_unique<Storage>(bytes, flags, mode, std::move(domain));
    }
    J3dMaterialTableData::~J3dMaterialTableData() {
        compat::JkrHostAllocationScope host;
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
