#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Map/RaceManager.hpp"
#include "Game/NPC/TalkMessageInfo.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <cstdio>

#define MESSAGE_ID_BUFFER_SIZE 256

namespace MR {
    const wchar_t* getSystemMessageDirect(const char* pMessageId) {
        TalkMessageInfo messageInfo = TalkMessageInfo();

        MessageSystem::getSystemMessageDirect(&messageInfo, pMessageId);

        return reinterpret_cast< wchar_t* >(messageInfo._0);
    }

    const wchar_t* getGameMessageDirect(const char* pMessageId) {
        TalkMessageInfo messageInfo = TalkMessageInfo();

        MessageSystem::getGameMessageDirect(&messageInfo, pMessageId);

        return reinterpret_cast< wchar_t* >(messageInfo._0);
    }

    const wchar_t* getLayoutMessageDirect(const char* pMessageId) {
        TalkMessageInfo messageInfo = TalkMessageInfo();

        MessageSystem::getLayoutMessageDirect(&messageInfo, pMessageId);

        return reinterpret_cast< wchar_t* >(messageInfo._0);
    }

    const wchar_t* getCurrentGalaxyNameOnCurrentLanguage() {
        return getGalaxyNameOnCurrentLanguage(getCurrentStageName());
    }

    const wchar_t* getCurrentGalaxyNameShortOnCurrentLanguage() {
        return getGalaxyNameShortOnCurrentLanguage(getCurrentStageName());
    }

    const wchar_t* getCurrentScenarioNameOnCurrentLanguage() {
        s32 selectedScenarioNo = getCurrentSelectedScenarioNo();
        s32 scenarioNo = selectedScenarioNo != -1 ? selectedScenarioNo : getCurrentScenarioNo();

        return getScenarioNameOnCurrentLanguage(getCurrentStageName(), scenarioNo);
    }

    const wchar_t* getRaceNameOnCurrentLanguage(int raceId) {
        return getGameMessageDirect(RaceManagerFunction::getRaceMessageId(raceId));
    }

    bool isExistGameMessage(const char* pMessageId) {
        TalkMessageInfo messageInfo = TalkMessageInfo();

        return MessageSystem::getGameMessageDirect(&messageInfo, pMessageId) &&
               getStringLengthWithMessageTag(reinterpret_cast< wchar_t* >(messageInfo._0)) != 0;
    }

    // getMessageLine
    s32 countMessageLine(const wchar_t* pMessage) {
        s32 count = 1;
        while (*pMessage != 0) {
            if (*pMessage == 0x1A) {
                pMessage++;
                MessageEditorMessageTag tag(pMessage);
                pMessage += tag.getSkipLength();
                if (tag.isGroupTagId(1, 1)) {
                    break;
                }
            } else {
                if (*pMessage == L'\n') {
                    count++;
                }
                pMessage++;
            }
        }
        return count;
    }
    s32 countMessageChar(const wchar_t* pMessage) {
        if (pMessage == nullptr) {
            return 0;
        }

        s32 count = 0;
        while (*pMessage != 0) {
            if (*pMessage == 0x1A) {
                pMessage++;
                MessageEditorMessageTag tag(pMessage);
                pMessage += tag.getSkipLength();
                u8 group = reinterpret_cast< const u8* >(tag.mMessage)[1];

                if (group == 3) {
                    count++;
                } else if (group == 6) {
                    count += countMessageFigure(*reinterpret_cast< const s32* >(tag.getParamPtr(0)));
                } else if (group == 5) {
                    count += 3;
                } else if (group == 11) {
                    count += 2;
                } else if (group == 7) {
                    count += countMessageChar(*reinterpret_cast< const wchar_t* const* >(tag.getParamPtr(0)));
                } else if (tag.isGroupTagId(1, 1)) {
                    break;
                }
            } else {
                pMessage++;
                count++;
            }
        }
        return count;
    }

    s32 countMessageFigure(s32 value) {
        u32 magnitude = MR::abs(value);
        s32 count = 1;
        while ((magnitude /= 10) != 0) {
            count++;
        }
        return count;
    }
    const wchar_t* getNextMessagePage(const wchar_t* pMessage) {
        while (*pMessage != 0) {
            if (*pMessage == 0x1A) {
                pMessage++;
                MessageEditorMessageTag tag(pMessage);
                pMessage += tag.getSkipLength();

                if (tag.isGroupTagId(1, 1)) {
                    if (*pMessage == L'\n') {
                        pMessage++;
                    }

                    return pMessage;
                }
            } else {
                pMessage++;
            }
        }

        return nullptr;
    }

    const wchar_t* getGalaxyNameOnCurrentLanguage(const char* pGalaxyName) {
        char messageId[MESSAGE_ID_BUFFER_SIZE];
        snprintf(messageId, sizeof(messageId), "GalaxyName_%s", pGalaxyName);

        return getGameMessageDirect(messageId);
    }

    const wchar_t* getGalaxyNameShortOnCurrentLanguage(const char* pGalaxyName) {
        char messageId[MESSAGE_ID_BUFFER_SIZE];
        snprintf(messageId, sizeof(messageId), "GalaxyNameShort_%s", pGalaxyName);

        return getGameMessageDirect(messageId);
    }

    const wchar_t* getScenarioNameOnCurrentLanguage(const char* pGalaxyName, s32 scenarioNo) {
        char messageId[MESSAGE_ID_BUFFER_SIZE];
        snprintf(messageId, sizeof(messageId), "ScenarioName_%s%d", pGalaxyName, scenarioNo);

        return getGameMessageDirect(messageId);
    }

    void getLayoutMessageID(char* pDst, const char* pSuperMessageId, const char* pSubMessageId) {
        snprintf(pDst, MESSAGE_ID_BUFFER_SIZE, "Layout_%s%s", pSuperMessageId, pSubMessageId);
    }

    void makeCometMessageID(char* pDst, u32 bufferSize, const char* pCometName) {
        snprintf(pDst, bufferSize, "CometName_%s", pCometName);
    }
};  // namespace MR

bool MessageEditorMessageTag::isGroupTagId(int group, int tag) const {
    return reinterpret_cast< const u8* >(mMessage)[1] == group && mMessage[1] == tag;
}
