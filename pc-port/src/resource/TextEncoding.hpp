#pragma once

#include <string>
#include <string_view>

namespace smgpc::resource {

    [[nodiscard]] std::u16string utf16_from_utf8_lossy(std::string_view text);
    [[nodiscard]] std::string utf8_from_utf16_lossy(std::u16string_view text);

    // Nintendo-authored BCSV/JMap strings use Windows-31J (CP932). Decode at
    // the resource boundary so compatibility consumers compare UTF-8 text.
    [[nodiscard]] std::string decode_cp932(std::string_view value);

}  // namespace smgpc::resource
