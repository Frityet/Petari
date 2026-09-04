#pragma once

#include <string>
#include <string_view>

namespace smgpc::resource {

    [[nodiscard]] std::u16string utf16_from_utf8_lossy(std::string_view text);
    [[nodiscard]] std::string utf8_from_utf16_lossy(std::u16string_view text);

    // Nintendo-authored BCSV/JMap strings use Windows-31J (CP932). Preserve
    // those bytes for Game identity; decode explicitly for UTF-8 presentation.
    [[nodiscard]] std::string decode_cp932(std::string_view value);

    // Convert host UTF-8 text explicitly before passing it to Game APIs.
    // Invalid UTF-8 and characters without a CP932 representation throw;
    // replacement characters and best-fit substitutions would change identity.
    [[nodiscard]] std::string encode_cp932(std::string_view value);

}  // namespace smgpc::resource
