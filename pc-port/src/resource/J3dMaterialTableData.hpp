#pragma once

#include <cstdint>
#include <memory>
#include <span>

class J3DMaterialTable;
namespace smgpc::compat { class JkrAllocationDomain; }

namespace smgpc::resource {
    // Retained v26 material construction. Uses original material factories and
    // loader sequences, with explicit native backing and allocation ownership.
    class J3dMaterialTableData final {
    public:
        enum class Mode { Model, BinaryModel, MaterialTable };

        J3dMaterialTableData(std::span<const std::uint8_t> complete_file,
                            std::uint32_t flags, Mode mode,
                            std::shared_ptr<compat::JkrAllocationDomain>);
        ~J3dMaterialTableData();
        J3dMaterialTableData(const J3dMaterialTableData&) = delete;
        J3dMaterialTableData& operator=(const J3dMaterialTableData&) = delete;

        // Attach once to fresh material fields; TEX1 retains independent ownership.
        // Does not populate or overwrite mTexture or mTextureName.
        void attach_to(J3DMaterialTable&);

        // Exact final MDL3 setter input for pre-indexToPtr validation. MAT3-only
        // materials begin at zero. This is construction metadata, not a live
        // observation after Game code changes a material's patch offsets.
        [[nodiscard]] std::uint32_t tex_no_patch_offset(std::uint16_t material_index) const;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
