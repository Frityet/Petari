#include "compat/FileSelectFuncCompat.hpp"

#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"

namespace smgpc::game::compat {

    void copy_file_select_fellow_name(u16* pName, const char* pMessageId, u32 length) {
        if (pName == nullptr || length == 0U) {
            return;
        }

        const wchar_t* pMessage = MR::getGameMessageDirect(pMessageId);

        u32 i = 0;
        if (pMessage != nullptr) {
            for (; i + 1U < length && pMessage[i] != L'\0'; ++i) {
                const auto codepoint = static_cast< unsigned long >(pMessage[i]);
                pName[i] = codepoint <= 0xFFFFUL ? static_cast< u16 >(codepoint) : static_cast< u16 >('?');
            }
        }

        for (; i < length; ++i) {
            pName[i] = 0U;
        }
    }

    void copy_file_select_mii_name(u16* pName, u16, u32 length) {
        if (pName != nullptr) {
            MR::zeroMemory(pName, length * sizeof(u16));
        }
    }

}  // namespace smgpc::game::compat
