#pragma once

namespace MR {
    const wchar_t* getSystemMessageDirect(const char* pMessageId);
    const wchar_t* getGameMessageDirect(const char* pMessageId);
    const wchar_t* getLayoutMessageDirect(const char* pMessageId);
    const wchar_t* getGalaxyNameOnCurrentLanguage(const char* pGalaxyName);
    bool isExistGameMessage(const char* pMessageId);
}
