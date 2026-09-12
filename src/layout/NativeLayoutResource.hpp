#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace smgpc::layout {

    // Native SDK record storage; the original Layout/AnimResource still walk
    // their original block graphs and construct the original SDK objects.
    class NativeLayoutResource final {
    public:
        [[nodiscard]] static std::shared_ptr<const NativeLayoutResource> create(const void* resource, std::uint32_t signature);
        [[nodiscard]] const void* data() const noexcept { return _bytes.data(); }
        [[nodiscard]] const void* source_identity() const noexcept { return _source; }
        static void validate_archive_span(std::span<const std::uint8_t> bytes);

        NativeLayoutResource(std::span<const std::uint8_t> bytes, const void* source, bool packed);

    private:
        std::vector<std::uint8_t> _bytes;
        const void* _source;
    };

}  // namespace smgpc::layout
