#pragma once

#include <revolution/types.h>

namespace MR {
    // Native clients that explicitly need the original UTF-16 code-unit width.
    const u16* getGameMessageDirectUtf16(const char*);
    const u16* getSystemMessageDirectUtf16(const char*);
}

namespace smgpc::compat {

    [[nodiscard]] const char*
    layout_message_id_for_pointer(const wchar_t* message) noexcept;

}  // namespace smgpc::compat
