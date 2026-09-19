#pragma once

#include <cstdint>
#include <string_view>

namespace smgpc::compat {
    // Explicit language metadata for a standalone resource owner. Original
    // GameSystem language remains authoritative whenever that process exists.
    // Publication follows the same serialized guest-owner lifetime as messages.
    class LanguageOwnership final {
    public:
        explicit LanguageOwnership(std::string_view original_prefix);
        ~LanguageOwnership();
        LanguageOwnership(const LanguageOwnership&) = delete;
        LanguageOwnership& operator=(const LanguageOwnership&) = delete;
        [[nodiscard]] static std::uint32_t require_language();

    private:
        std::uint32_t _language;
        LanguageOwnership* _previous;
    };
}
