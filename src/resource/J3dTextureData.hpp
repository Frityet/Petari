#pragma once

#include <cstdint>
#include <memory>
#include <span>

class J3DTexture;
class J3DMaterialTable;
class JUTNameTab;

namespace smgpc::resource {
    class Mem1ResourceHeap;

    // Retained TEX1 construction component. Native ResTIMG records and their
    // unchanged source payloads live in one shared-heap MEM1 allocation.
    class J3dTextureData final {
    public:
        J3dTextureData(std::span<const std::uint8_t> tex1, std::shared_ptr<Mem1ResourceHeap> heap);
        ~J3dTextureData();
        J3dTextureData(J3dTextureData&&) noexcept;
        J3dTextureData& operator=(J3dTextureData&&) = delete;
        J3dTextureData(const J3dTextureData&) = delete;
        J3dTextureData& operator=(const J3dTextureData&) = delete;

        [[nodiscard]] J3DTexture& texture() const noexcept;
        [[nodiscard]] JUTNameTab* names() const noexcept;
        [[nodiscard]] std::span<const std::uint8_t> source_bytes() const noexcept;
        // Attach once to an unpopulated texture table. The complete model owner
        // must retain this component for every use of that table/display list.
        void attach_to(J3DMaterialTable&);

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
