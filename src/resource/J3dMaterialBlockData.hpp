#pragma once

#include <cstdint>
#include <memory>
#include <span>

struct J3DMaterialBlock;
struct J3DMaterialDLBlock;

namespace smgpc::resource {
    // Typed, retained factory input. The caller supplies one complete MAT3 or
    // MDL3 block, then uses the original J3DMaterialFactory with its native header.
    // This component does not publish a model or create material instances.
    class J3dMaterialBlockData final {
    public:
        explicit J3dMaterialBlockData(std::span<const std::uint8_t> block);
        ~J3dMaterialBlockData();
        J3dMaterialBlockData(const J3dMaterialBlockData&) = delete;
        J3dMaterialBlockData& operator=(const J3dMaterialBlockData&) = delete;
        [[nodiscard]] const J3DMaterialBlock& material() const;
        [[nodiscard]] const J3DMaterialDLBlock& display_list() const;
        [[nodiscard]] std::span<const std::uint8_t> source_bytes() const;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
