#pragma once

#include "compat/Types.hpp"

namespace smgpc::game::compat {

    void copy_file_select_fellow_name(u16* pName, const char* pMessageId, u32 length);
    void copy_file_select_mii_name(u16* pName, u16 miiIndex, u32 length);

}  // namespace smgpc::game::compat
