#include "Game/Util/StringUtil.hpp"

#include "Game/Screen/ReplaceTagProcessor.hpp"
#include "Game/Util/MessageUtil.hpp"

namespace MR {
    void makeDateString(wchar_t* pDst, s32 size, s32 year, s32 month, s32 day) {
        const wchar_t* pMessage = MR::getGameMessageDirect("System_Date000");
        ReplaceTagFunction::ReplaceArgs(pDst, size, pMessage, year, month, day);
    }

    void makeTimeString(wchar_t* pDst, s32 size, s32 hour, s32 minute) {
        const wchar_t* pMessage = MR::getGameMessageDirect("System_Time002");
        ReplaceTagFunction::ReplaceArgs(pDst, size, pMessage, hour, minute);
    }
}  // namespace MR
