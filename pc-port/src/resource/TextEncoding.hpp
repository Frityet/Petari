#pragma once

#include <string>
#include <string_view>

namespace smgpc::resource {

    [[nodiscard]] std::u16string utf16_from_utf8_lossy(std::string_view text);
    [[nodiscard]] std::string utf8_from_utf16_lossy(std::u16string_view text);

}  // namespace smgpc::resource
