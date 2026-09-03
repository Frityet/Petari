#pragma once

#include <cstdint>
#include <memory>
#include <span>

class J3DModelData;
struct J3DShapeBlock;

namespace smgpc::resource {

    // Retained VTX1/SHP1 construction data and actual original shape instances.
    // The complete model owner must retain this component, validate cross-table
    // matrix/material references, and perform original hierarchy finalization.
    // This component does not publish a complete model or synthesize materials.
    class J3dGeometryData final {
    public:
        explicit J3dGeometryData(std::span<const std::uint8_t> model_bytes,
                                 std::uint32_t load_flags = 0);
        ~J3dGeometryData();
        J3dGeometryData(J3dGeometryData&&) noexcept;
        J3dGeometryData& operator=(J3dGeometryData&&) = delete;
        J3dGeometryData(const J3dGeometryData&) = delete;
        J3dGeometryData& operator=(const J3dGeometryData&) = delete;

        // Attach once to fresh vertex/shape tables. INF packet/vertex counts are
        // retained; all pointers remain valid until this component is destroyed.
        // Does not link materials, make VCD/VAT commands, or sort command aliases.
        void attach_to(J3DModelData& model_data);

        // Original factory input with native, header-relative metadata offsets.
        // Its mBlockSize remains the authored size, not a native allocation size.
        [[nodiscard]] const J3DShapeBlock& shape_block() const;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

}  // namespace smgpc::resource
