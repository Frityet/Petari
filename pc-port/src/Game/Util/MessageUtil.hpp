#pragma once

namespace MR {
    const wchar_t* getSystemMessageDirect(const char* pMessageId);
    const wchar_t* getGameMessageDirect(const char* pMessageId);
    const wchar_t* getLayoutMessageDirect(const char* pMessageId);
    bool isExistGameMessage(const char* pMessageId);
}
