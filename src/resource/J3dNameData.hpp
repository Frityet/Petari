#pragma once

#include <cstdint>
#include <memory>
#include <span>

struct ResNTAB;
class JUTNameTab;

namespace smgpc::resource {

    // A retained original name table. Pass bytes beginning at the ResNTAB
    // header, bounded by its containing resource block. The default owner
    // represents an absent table; a present table may have zero entries.
    class J3dNameData final {
    public:
        J3dNameData() noexcept;
        explicit J3dNameData(std::span<const std::uint8_t> bytes);
        ~J3dNameData();
        J3dNameData(J3dNameData&&) noexcept;
        J3dNameData& operator=(J3dNameData&&) noexcept;
        J3dNameData(const J3dNameData&) = delete;
        J3dNameData& operator=(const J3dNameData&) = delete;

        [[nodiscard]] const ResNTAB* resource() const noexcept;
        [[nodiscard]] JUTNameTab* table() const noexcept;

        // Native-endian header/entries with unchanged relative string offsets
        // and bytes. Suitable for copying into another native metadata arena
        // at alignof(ResNTAB). A zero-entry source may include unused tail
        // padding so that an entire native ResNTAB object fits its allocation.
        [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

}  // namespace smgpc::resource
