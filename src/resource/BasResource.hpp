#pragma once

#include <cstdint>
#include <memory>
#include <span>

class JAUSoundAnimation;

namespace smgpc::resource {
// Retains the archive and converts only the pointer-free BAS disk records.
// Its real JAUSoundAnimationControl owns the native event array; no JAISound
// instance, handle attachment, or playback object is synthesized.
class BasResource final {
public:
    BasResource(std::span<const std::uint8_t>, std::shared_ptr<const void> archive_owner);
    ~BasResource();
    BasResource(BasResource&&) noexcept;
    BasResource& operator=(BasResource&&) noexcept;
    BasResource(const BasResource&) = delete;
    BasResource& operator=(const BasResource&) = delete;
    [[nodiscard]] const JAUSoundAnimation* animation() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> _storage;
};

[[nodiscard]] const JAUSoundAnimation* resolve_bas_animation(const JAUSoundAnimation*);
}
