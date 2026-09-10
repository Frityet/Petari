#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace smgpc::resource {
    // Retains an original-layout BMG with native scalar fields and one wchar_t
    // per authored UTF-16 code unit. DAT1 offsets are relocated, not decoded as
    // Unicode scalar values: control-tag lengths and surrogate units stay intact.
    class NativeBmgResource final {
    public:
        NativeBmgResource(std::span<const std::uint8_t> bmg,
                          std::span<const std::uint8_t> message_ids);
        ~NativeBmgResource();
        NativeBmgResource(const NativeBmgResource&) = delete;
        NativeBmgResource& operator=(const NativeBmgResource&) = delete;
        [[nodiscard]] std::uint8_t* data() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] std::size_t message_count() const noexcept;
        [[nodiscard]] const wchar_t* message(std::size_t index) const;
        [[nodiscard]] const char16_t* message_utf16(std::size_t index) const;
        [[nodiscard]] std::optional<std::size_t> message_index(const wchar_t*) const noexcept;
    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    [[nodiscard]] std::unique_ptr<NativeBmgResource> make_native_bmg_resource(
        const void* bmg, std::size_t bmg_size,
        const void* message_ids, std::size_t message_ids_size);
}
